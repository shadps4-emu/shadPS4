// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/intrusive/list.hpp>
#include <fmt/format.h>
#include "shader_recompiler/frontend/structured_control_flow.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/ir/ir_emitter.h"

namespace Shader::Gcn {

namespace {

struct Statement;

// Use normal_link because we are not guaranteed to destroy the tree in order
using ListBaseHook =
    boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>;

using Tree = boost::intrusive::list<Statement,
                                    // Allow using Statement without a definition
                                    boost::intrusive::base_hook<ListBaseHook>,
                                    // Avoid linear complexity on splice, size is never called
                                    boost::intrusive::constant_time_size<false>>;
using Node = Tree::iterator;

enum class StatementType {
    Code,
    Goto,
    Label,
    If,
    Loop,
    Break,
    Return,
    Unreachable,
    Function,
    Identity,
    Not,
    Or,
    SetVariable,
    Variable,
};

bool HasChildren(StatementType type) {
    switch (type) {
    case StatementType::If:
    case StatementType::Loop:
    case StatementType::Function:
        return true;
    default:
        return false;
    }
}

struct Goto {};
struct Label {};
struct If {};
struct Loop {};
struct Break {};
struct Return {};
struct Kill {};
struct Unreachable {};
struct FunctionTag {};
struct Identity {};
struct Not {};
struct Or {};
struct SetVariable {};
struct Variable {};

struct Statement : ListBaseHook {
    Statement(const Block* block_, Statement* up_)
        : block{block_}, up{up_}, type{StatementType::Code} {}
    Statement(Goto, Statement* cond_, Node label_, Statement* up_)
        : label{label_}, cond{cond_}, up{up_}, type{StatementType::Goto} {}
    Statement(Label, u32 id_, Statement* up_) : id{id_}, up{up_}, type{StatementType::Label} {}
    Statement(If, Statement* cond_, Tree&& children_, Statement* up_)
        : children{std::move(children_)}, cond{cond_}, up{up_}, type{StatementType::If} {}
    Statement(Loop, Statement* cond_, Tree&& children_, Statement* up_)
        : children{std::move(children_)}, cond{cond_}, up{up_}, type{StatementType::Loop} {}
    Statement(Break, Statement* cond_, Statement* up_)
        : cond{cond_}, up{up_}, type{StatementType::Break} {}
    Statement(Return, Statement* up_) : up{up_}, type{StatementType::Return} {}
    Statement(Unreachable, Statement* up_) : up{up_}, type{StatementType::Unreachable} {}
    Statement(FunctionTag) : children{}, type{StatementType::Function} {}
    Statement(Identity, IR::Condition cond_, Statement* up_)
        : guest_cond{cond_}, up{up_}, type{StatementType::Identity} {}
    Statement(Not, Statement* op_, Statement* up_) : op{op_}, up{up_}, type{StatementType::Not} {}
    Statement(Or, Statement* op_a_, Statement* op_b_, Statement* up_)
        : op_a{op_a_}, op_b{op_b_}, up{up_}, type{StatementType::Or} {}
    Statement(SetVariable, u32 id_, Statement* op_, Statement* up_)
        : op{op_}, id{id_}, up{up_}, type{StatementType::SetVariable} {}
    Statement(Variable, u32 id_, Statement* up_)
        : id{id_}, up{up_}, type{StatementType::Variable} {}

    ~Statement() {
        if (HasChildren(type)) {
            std::destroy_at(&children);
        }
    }

    union {
        const Block* block;
        Node label;
        Tree children;
        IR::Condition guest_cond;
        Statement* op;
        Statement* op_a;
        u32 location;
        s32 branch_offset;
    };
    union {
        Statement* cond;
        Statement* op_b;
        u32 id;
    };
    Statement* up{};
    StatementType type;
};

std::string DumpExpr(const Statement* stmt) {
    switch (stmt->type) {
    case StatementType::Identity:
        return fmt::format("{}", stmt->guest_cond);
    case StatementType::Not:
        return fmt::format("!{}", DumpExpr(stmt->op));
    case StatementType::Or:
        return fmt::format("{} || {}", DumpExpr(stmt->op_a), DumpExpr(stmt->op_b));
    case StatementType::Variable:
        return fmt::format("goto_L{}", stmt->id);
    default:
        return "<invalid type>";
    }
}

[[maybe_unused]] std::string DumpTree(const Tree& tree, u32 indentation = 0) {
    std::string ret;
    std::string indent(indentation, ' ');
    for (const auto& stmt : tree) {
        switch (stmt.type) {
        case StatementType::Code:
            ret += fmt::format("{}    Block {:04x} -> {:04x} (0x{:016x});\n", indent,
                               stmt.block->begin, stmt.block->end,
                               reinterpret_cast<uintptr_t>(stmt.block));
            break;
        case StatementType::Goto:
            ret += fmt::format("{}    if ({}) goto L{};\n", indent, DumpExpr(stmt.cond),
                               stmt.label->id);
            break;
        case StatementType::Label:
            ret += fmt::format("{}L{}:\n", indent, stmt.id);
            break;
        case StatementType::If:
            ret += fmt::format("{}    if ({}) {{\n", indent, DumpExpr(stmt.cond));
            ret += DumpTree(stmt.children, indentation + 4);
            ret += fmt::format("{}    }}\n", indent);
            break;
        case StatementType::Loop:
            ret += fmt::format("{}    do {{\n", indent);
            ret += DumpTree(stmt.children, indentation + 4);
            ret += fmt::format("{}    }} while ({});\n", indent, DumpExpr(stmt.cond));
            break;
        case StatementType::Break:
            ret += fmt::format("{}    if ({}) break;\n", indent, DumpExpr(stmt.cond));
            break;
        case StatementType::Return:
            ret += fmt::format("{}    return;\n", indent);
            break;
        case StatementType::Unreachable:
            ret += fmt::format("{}    unreachable;\n", indent);
            break;
        case StatementType::SetVariable:
            ret += fmt::format("{}    goto_L{} = {};\n", indent, stmt.id, DumpExpr(stmt.op));
            break;
        case StatementType::Function:
        case StatementType::Identity:
        case StatementType::Not:
        case StatementType::Or:
        case StatementType::Variable:
            UNREACHABLE_MSG("Statement can't be printed");
        }
    }
    return ret;
}

void SanitizeNoBreaks(const Tree& tree) {
    if (std::ranges::find(tree, StatementType::Break, &Statement::type) != tree.end()) {
        UNREACHABLE_MSG("Capturing statement with break nodes");
    }
}

size_t Level(Node stmt) {
    size_t level{0};
    Statement* node{stmt->up};
    while (node) {
        ++level;
        node = node->up;
    }
    return level;
}

bool IsDirectlyRelated(Node goto_stmt, Node label_stmt) {
    const size_t goto_level{Level(goto_stmt)};
    const size_t label_level{Level(label_stmt)};
    size_t min_level;
    size_t max_level;
    Node min;
    Node max;
    if (label_level < goto_level) {
        min_level = label_level;
        max_level = goto_level;
        min = label_stmt;
        max = goto_stmt;
    } else { // goto_level < label_level
        min_level = goto_level;
        max_level = label_level;
        min = goto_stmt;
        max = label_stmt;
    }
    while (max_level > min_level) {
        --max_level;
        max = max->up;
    }
    return min->up == max->up;
}

bool IsIndirectlyRelated(Node goto_stmt, Node label_stmt) {
    return goto_stmt->up != label_stmt->up && !IsDirectlyRelated(goto_stmt, label_stmt);
}

[[maybe_unused]] bool AreSiblings(Node goto_stmt, Node label_stmt) noexcept {
    Node it{goto_stmt};
    do {
        if (it == label_stmt) {
            return true;
        }
        --it;
    } while (it != goto_stmt->up->children.begin());
    while (it != goto_stmt->up->children.end()) {
        if (it == label_stmt) {
            return true;
        }
        ++it;
    }
    return false;
}

Node SiblingFromNephew(Node uncle, Node nephew) noexcept {
    Statement* const parent{uncle->up};
    Statement* it{&*nephew};
    while (it->up != parent) {
        it = it->up;
    }
    return Tree::s_iterator_to(*it);
}

bool AreOrdered(Node left_sibling, Node right_sibling) noexcept {
    const Node end{right_sibling->up->children.end()};
    for (auto it = right_sibling; it != end; ++it) {
        if (it == left_sibling) {
            return false;
        }
    }
    return true;
}

bool NeedsLift(Node goto_stmt, Node label_stmt) noexcept {
    const Node sibling{SiblingFromNephew(goto_stmt, label_stmt)};
    return AreOrdered(sibling, goto_stmt);
}

/**
 * The algorithm used here is from:
 * Taming Control Flow: A Structured Approach to Eliminating Goto Statements.
 * Ana M. Erosa and Laurie J. Hendren
 * http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.1485&rep=rep1&type=pdf
 */
class GotoPass {
public:
    explicit GotoPass(CFG& cfg, Common::ObjectPool<Statement>& stmt_pool) : pool{stmt_pool} {
        std::vector gotos{BuildTree(cfg)};
        const auto end{gotos.rend()};
        for (auto goto_stmt = gotos.rbegin(); goto_stmt != end; ++goto_stmt) {
            RemoveGoto(*goto_stmt);
        }
    }

    Statement& RootStatement() noexcept {
        return root_stmt;
    }

private:
    void RemoveGoto(Node goto_stmt) {
        // Force goto_stmt and label_stmt to be directly related
        const Node label_stmt{goto_stmt->label};
        if (IsIndirectlyRelated(goto_stmt, label_stmt)) {
            // Move goto_stmt out using outward-movement transformation until it becomes
            // directly related to label_stmt
            while (!IsDirectlyRelated(goto_stmt, label_stmt)) {
                goto_stmt = MoveOutward(goto_stmt);
            }
        }
        // Force goto_stmt and label_stmt to be siblings
        if (IsDirectlyRelated(goto_stmt, label_stmt)) {
            const size_t label_level{Level(label_stmt)};
            size_t goto_level{Level(goto_stmt)};
            if (goto_level > label_level) {
                // Move goto_stmt out of its level using outward-movement transformations
                while (goto_level > label_level) {
                    goto_stmt = MoveOutward(goto_stmt);
                    --goto_level;
                }
            } else { // Level(goto_stmt) < Level(label_stmt)
                if (NeedsLift(goto_stmt, label_stmt)) {
                    // Lift goto_stmt to above stmt containing label_stmt using goto-lifting
                    // transformations
                    goto_stmt = Lift(goto_stmt);
                }
                // Move goto_stmt into label_stmt's level using inward-movement transformation
                while (goto_level < label_level) {
                    goto_stmt = MoveInward(goto_stmt);
                    ++goto_level;
                }
            }
        }
        // Expensive operation:
        // if (!AreSiblings(goto_stmt, label_stmt)) {
        //    UNREACHABLE_MSG("Goto is not a sibling with the label");
        //}
        // goto_stmt and label_stmt are guaranteed to be siblings, eliminate
        if (std::next(goto_stmt) == label_stmt) {
            // Simply eliminate the goto if the label is next to it
            goto_stmt->up->children.erase(goto_stmt);
        } else if (AreOrdered(goto_stmt, label_stmt)) {
            // Eliminate goto_stmt with a conditional
            EliminateAsConditional(goto_stmt, label_stmt);
        } else {
            // Eliminate goto_stmt with a loop
            EliminateAsLoop(goto_stmt, label_stmt);
        }
    }

    std::vector<Node> BuildTree(CFG& cfg) {
        u32 label_id{0};
        std::vector<Node> gotos;
        BuildTree(cfg, label_id, gotos, root_stmt.children.end(), std::nullopt);
        return gotos;
    }

    void BuildTree(CFG& cfg, u32& label_id, std::vector<Node>& gotos, Node function_insert_point,
                   std::optional<Node> return_label) {
        Statement* const false_stmt{pool.Create(Identity{}, IR::Condition::False, &root_stmt)};
        Tree& root{root_stmt.children};
        std::unordered_map<Block*, Node> local_labels;
        local_labels.reserve(cfg.blocks.size());

        for (Block& block : cfg.blocks) {
            Statement* const label{pool.Create(Label{}, label_id, &root_stmt)};
            const Node label_it{root.insert(function_insert_point, *label)};
            local_labels.emplace(&block, label_it);
            ++label_id;
        }
        for (Block& block : cfg.blocks) {
            const Node label{local_labels.at(&block)};
            // Insertion point
            const Node ip{std::next(label)};

            // Reset goto variables before the first block and after its respective label
            const auto make_reset_variable{[&]() -> Statement& {
                return *pool.Create(SetVariable{}, label->id, false_stmt, &root_stmt);
            }};
            root.push_front(make_reset_variable());
            root.insert(ip, make_reset_variable());
            root.insert(ip, *pool.Create(&block, &root_stmt));

            switch (block.end_class) {
            case EndClass::Branch: {
                Statement* const always_cond{
                    pool.Create(Identity{}, IR::Condition::True, &root_stmt)};
                if (block.cond == IR::Condition::True) {
                    const Node true_label{local_labels.at(block.branch_true)};
                    gotos.push_back(
                        root.insert(ip, *pool.Create(Goto{}, always_cond, true_label, &root_stmt)));
                } else if (block.cond == IR::Condition::False) {
                    const Node false_label{local_labels.at(block.branch_false)};
                    gotos.push_back(root.insert(
                        ip, *pool.Create(Goto{}, always_cond, false_label, &root_stmt)));
                } else {
                    const Node true_label{local_labels.at(block.branch_true)};
                    const Node false_label{local_labels.at(block.branch_false)};
                    Statement* const true_cond{pool.Create(Identity{}, block.cond, &root_stmt)};
                    gotos.push_back(
                        root.insert(ip, *pool.Create(Goto{}, true_cond, true_label, &root_stmt)));
                    gotos.push_back(root.insert(
                        ip, *pool.Create(Goto{}, always_cond, false_label, &root_stmt)));
                }
                break;
            }
            case EndClass::Exit:
                root.insert(ip, *pool.Create(Return{}, &root_stmt));
                break;
            }
        }
    }

    void UpdateTreeUp(Statement* tree) {
        for (Statement& stmt : tree->children) {
            stmt.up = tree;
        }
    }

    void EliminateAsConditional(Node goto_stmt, Node label_stmt) {
        Tree& body{goto_stmt->up->children};
        Tree if_body;
        if_body.splice(if_body.begin(), body, std::next(goto_stmt), label_stmt);
        Statement* const cond{pool.Create(Not{}, goto_stmt->cond, &root_stmt)};
        Statement* const if_stmt{pool.Create(If{}, cond, std::move(if_body), goto_stmt->up)};
        UpdateTreeUp(if_stmt);
        body.insert(goto_stmt, *if_stmt);
        body.erase(goto_stmt);
    }

    void EliminateAsLoop(Node goto_stmt, Node label_stmt) {
        Tree& body{goto_stmt->up->children};
        Tree loop_body;
        loop_body.splice(loop_body.begin(), body, label_stmt, goto_stmt);
        Statement* const cond{goto_stmt->cond};
        Statement* const loop{pool.Create(Loop{}, cond, std::move(loop_body), goto_stmt->up)};
        UpdateTreeUp(loop);
        body.insert(goto_stmt, *loop);
        body.erase(goto_stmt);
    }

    [[nodiscard]] Node MoveOutward(Node goto_stmt) {
        switch (goto_stmt->up->type) {
        case StatementType::If:
            return MoveOutwardIf(goto_stmt);
        case StatementType::Loop:
            return MoveOutwardLoop(goto_stmt);
        default:
            UNREACHABLE_MSG("Invalid outward movement");
        }
    }

    [[nodiscard]] Node MoveInward(Node goto_stmt) {
        Statement* const parent{goto_stmt->up};
        Tree& body{parent->children};
        const Node label{goto_stmt->label};
        const Node label_nested_stmt{SiblingFromNephew(goto_stmt, label)};
        const u32 label_id{label->id};

        Statement* const goto_cond{goto_stmt->cond};
        Statement* const set_var{pool.Create(SetVariable{}, label_id, goto_cond, parent)};
        body.insert(goto_stmt, *set_var);

        Tree if_body;
        if_body.splice(if_body.begin(), body, std::next(goto_stmt), label_nested_stmt);
        Statement* const variable{pool.Create(Variable{}, label_id, &root_stmt)};
        Statement* const neg_var{pool.Create(Not{}, variable, &root_stmt)};
        if (!if_body.empty()) {
            Statement* const if_stmt{pool.Create(If{}, neg_var, std::move(if_body), parent)};
            UpdateTreeUp(if_stmt);
            body.insert(goto_stmt, *if_stmt);
        }
        body.erase(goto_stmt);

        switch (label_nested_stmt->type) {
        case StatementType::If:
            // Update nested if condition
            label_nested_stmt->cond =
                pool.Create(Or{}, variable, label_nested_stmt->cond, &root_stmt);
            break;
        case StatementType::Loop:
            break;
        default:
            UNREACHABLE_MSG("Invalid inward movement");
        }
        Tree& nested_tree{label_nested_stmt->children};
        Statement* const new_goto{pool.Create(Goto{}, variable, label, &*label_nested_stmt)};
        return nested_tree.insert(nested_tree.begin(), *new_goto);
    }

    [[nodiscard]] Node Lift(Node goto_stmt) {
        Statement* const parent{goto_stmt->up};
        Tree& body{parent->children};
        const Node label{goto_stmt->label};
        const u32 label_id{label->id};
        const Node label_nested_stmt{SiblingFromNephew(goto_stmt, label)};

        Tree loop_body;
        loop_body.splice(loop_body.begin(), body, label_nested_stmt, goto_stmt);
        SanitizeNoBreaks(loop_body);
        Statement* const variable{pool.Create(Variable{}, label_id, &root_stmt)};
        Statement* const loop_stmt{pool.Create(Loop{}, variable, std::move(loop_body), parent)};
        UpdateTreeUp(loop_stmt);
        body.insert(goto_stmt, *loop_stmt);

        Statement* const new_goto{pool.Create(Goto{}, variable, label, loop_stmt)};
        loop_stmt->children.push_front(*new_goto);
        const Node new_goto_node{loop_stmt->children.begin()};

        Statement* const set_var{pool.Create(SetVariable{}, label_id, goto_stmt->cond, loop_stmt)};
        loop_stmt->children.push_back(*set_var);

        body.erase(goto_stmt);
        return new_goto_node;
    }

    Node MoveOutwardIf(Node goto_stmt) {
        const Node parent{Tree::s_iterator_to(*goto_stmt->up)};
        Tree& body{parent->children};
        const u32 label_id{goto_stmt->label->id};
        Statement* const goto_cond{goto_stmt->cond};
        Statement* const set_goto_var{pool.Create(SetVariable{}, label_id, goto_cond, &*parent)};
        body.insert(goto_stmt, *set_goto_var);

        Tree if_body;
        if_body.splice(if_body.begin(), body, std::next(goto_stmt), body.end());
        if_body.pop_front();
        Statement* const cond{pool.Create(Variable{}, label_id, &root_stmt)};
        Statement* const neg_cond{pool.Create(Not{}, cond, &root_stmt)};
        Statement* const if_stmt{pool.Create(If{}, neg_cond, std::move(if_body), &*parent)};
        UpdateTreeUp(if_stmt);
        body.insert(goto_stmt, *if_stmt);

        body.erase(goto_stmt);

        Statement* const new_cond{pool.Create(Variable{}, label_id, &root_stmt)};
        Statement* const new_goto{pool.Create(Goto{}, new_cond, goto_stmt->label, parent->up)};
        Tree& parent_tree{parent->up->children};
        return parent_tree.insert(std::next(parent), *new_goto);
    }

    Node MoveOutwardLoop(Node goto_stmt) {
        Statement* const parent{goto_stmt->up};
        Tree& body{parent->children};
        const u32 label_id{goto_stmt->label->id};
        Statement* const goto_cond{goto_stmt->cond};
        Statement* const set_goto_var{pool.Create(SetVariable{}, label_id, goto_cond, parent)};
        Statement* const cond{pool.Create(Variable{}, label_id, &root_stmt)};
        Statement* const break_stmt{pool.Create(Break{}, cond, parent)};
        body.insert(goto_stmt, *set_goto_var);
        body.insert(goto_stmt, *break_stmt);
        body.erase(goto_stmt);

        const Node loop{Tree::s_iterator_to(*goto_stmt->up)};
        Statement* const new_goto_cond{pool.Create(Variable{}, label_id, &root_stmt)};
        Statement* const new_goto{pool.Create(Goto{}, new_goto_cond, goto_stmt->label, loop->up)};
        Tree& parent_tree{loop->up->children};
        return parent_tree.insert(std::next(loop), *new_goto);
    }

    Common::ObjectPool<Statement>& pool;
    Statement root_stmt{FunctionTag{}};
};

[[nodiscard]] Statement* TryFindForwardBlock(Statement& stmt) {
    Tree& tree{stmt.up->children};
    const Node end{tree.end()};
    Node forward_node{std::next(Tree::s_iterator_to(stmt))};
    while (forward_node != end && !HasChildren(forward_node->type)) {
        if (forward_node->type == StatementType::Code) {
            return &*forward_node;
        }
        ++forward_node;
    }
    return nullptr;
}

[[nodiscard]] IR::U1 VisitExpr(IR::IREmitter& ir, const Statement& stmt) {
    switch (stmt.type) {
    case StatementType::Identity:
        return ir.Condition(stmt.guest_cond);
    case StatementType::Not:
        return ir.LogicalNot(IR::U1{VisitExpr(ir, *stmt.op)});
    case StatementType::Or:
        return ir.LogicalOr(VisitExpr(ir, *stmt.op_a), VisitExpr(ir, *stmt.op_b));
    case StatementType::Variable:
        return ir.GetGotoVariable(stmt.id);
    default:
        UNREACHABLE_MSG("Statement type {}", u32(stmt.type));
    }
}

class TranslatePass {
public:
    TranslatePass(Common::ObjectPool<IR::Inst>& inst_pool_,
                  Common::ObjectPool<IR::Block>& block_pool_,
                  Common::ObjectPool<Statement>& stmt_pool_, Statement& root_stmt,
                  IR::AbstractSyntaxList& syntax_list_, std::span<const GcnInst> inst_list_,
                  Info& info_, const RuntimeInfo& runtime_info_, const Profile& profile_)
        : stmt_pool{stmt_pool_}, inst_pool{inst_pool_}, block_pool{block_pool_},
          syntax_list{syntax_list_}, inst_list{inst_list_}, info{info_},
          runtime_info{runtime_info_}, profile{profile_},
          translator{info_, runtime_info_, profile_} {
        Visit(root_stmt, nullptr, nullptr);

        IR::Block* first_block = syntax_list.front().data.block;
        translator.EmitPrologue(first_block);
    }

private:
    void Visit(Statement& parent, IR::Block* break_block, IR::Block* fallthrough_block) {
        IR::Block* current_block{};
        const auto ensure_block{[&] {
            if (current_block) {
                return;
            }
            current_block = block_pool.Create(inst_pool);
            auto& node{syntax_list.emplace_back()};
            node.type = IR::AbstractSyntaxNode::Type::Block;
            node.data.block = current_block;
        }};
        Tree& tree{parent.children};
        for (auto& child : tree) {
            Statement& stmt{child};
            switch (stmt.type) {
            case StatementType::Label:
                // Labels can be ignored
                break;
            case StatementType::Code: {
                ensure_block();
                if (!stmt.block->is_dummy) {
                    const u32 start = stmt.block->begin_index;
                    const u32 size = stmt.block->end_index - start + 1;
                    current_block->cfg_block = stmt.block;
                    translator.Translate(current_block, stmt.block->begin,
                                         inst_list.subspan(start, size));
                }
                break;
            }
            case StatementType::SetVariable: {
                ensure_block();
                IR::IREmitter ir{*current_block};
                ir.SetGotoVariable(stmt.id, VisitExpr(ir, *stmt.op));
                break;
            }
            case StatementType::If: {
                ensure_block();
                IR::Block* const merge_block{MergeBlock(parent, stmt)};

                // Implement if header block
                IR::IREmitter ir{*current_block};
                const IR::U1 cond{ir.ConditionRef(VisitExpr(ir, *stmt.cond))};

                const size_t if_node_index{syntax_list.size()};
                syntax_list.emplace_back();

                // Visit children
                const size_t then_block_index{syntax_list.size()};
                Visit(stmt, break_block, merge_block);

                IR::Block* const then_block{syntax_list.at(then_block_index).data.block};
                current_block->AddBranch(then_block);
                current_block->AddBranch(merge_block);
                current_block = merge_block;

                auto& if_node{syntax_list[if_node_index]};
                if_node.type = IR::AbstractSyntaxNode::Type::If;
                if_node.data.if_node.cond = cond;
                if_node.data.if_node.body = then_block;
                if_node.data.if_node.merge = merge_block;

                auto& endif_node{syntax_list.emplace_back()};
                endif_node.type = IR::AbstractSyntaxNode::Type::EndIf;
                endif_node.data.end_if.merge = merge_block;

                auto& merge{syntax_list.emplace_back()};
                merge.type = IR::AbstractSyntaxNode::Type::Block;
                merge.data.block = merge_block;
                break;
            }
            case StatementType::Loop: {
                IR::Block* const loop_header_block{block_pool.Create(inst_pool)};
                if (current_block) {
                    current_block->AddBranch(loop_header_block);
                }
                auto& header_node{syntax_list.emplace_back()};
                header_node.type = IR::AbstractSyntaxNode::Type::Block;
                header_node.data.block = loop_header_block;

                IR::Block* const continue_block{block_pool.Create(inst_pool)};
                IR::Block* const merge_block{MergeBlock(parent, stmt)};

                const size_t loop_node_index{syntax_list.size()};
                syntax_list.emplace_back();

                // Visit children
                const size_t body_block_index{syntax_list.size()};
                Visit(stmt, merge_block, continue_block);

                // The continue block is located at the end of the loop
                IR::IREmitter ir{*continue_block};
                const IR::U1 cond{ir.ConditionRef(VisitExpr(ir, *stmt.cond))};

                IR::Block* const body_block{syntax_list.at(body_block_index).data.block};
                loop_header_block->AddBranch(body_block);

                continue_block->AddBranch(loop_header_block);
                continue_block->AddBranch(merge_block);

                current_block = merge_block;

                auto& loop{syntax_list[loop_node_index]};
                loop.type = IR::AbstractSyntaxNode::Type::Loop;
                loop.data.loop.body = body_block;
                loop.data.loop.continue_block = continue_block;
                loop.data.loop.merge = merge_block;

                auto& continue_block_node{syntax_list.emplace_back()};
                continue_block_node.type = IR::AbstractSyntaxNode::Type::Block;
                continue_block_node.data.block = continue_block;

                auto& repeat{syntax_list.emplace_back()};
                repeat.type = IR::AbstractSyntaxNode::Type::Repeat;
                repeat.data.repeat.cond = cond;
                repeat.data.repeat.loop_header = loop_header_block;
                repeat.data.repeat.merge = merge_block;

                auto& merge{syntax_list.emplace_back()};
                merge.type = IR::AbstractSyntaxNode::Type::Block;
                merge.data.block = merge_block;
                break;
            }
            case StatementType::Break: {
                ensure_block();
                IR::Block* const skip_block{MergeBlock(parent, stmt)};

                IR::IREmitter ir{*current_block};
                const IR::U1 cond{ir.ConditionRef(VisitExpr(ir, *stmt.cond))};
                current_block->AddBranch(break_block);
                current_block->AddBranch(skip_block);
                current_block = skip_block;

                auto& break_node{syntax_list.emplace_back()};
                break_node.type = IR::AbstractSyntaxNode::Type::Break;
                break_node.data.break_node.cond = cond;
                break_node.data.break_node.merge = break_block;
                break_node.data.break_node.skip = skip_block;

                auto& merge{syntax_list.emplace_back()};
                merge.type = IR::AbstractSyntaxNode::Type::Block;
                merge.data.block = skip_block;
                break;
            }
            case StatementType::Return: {
                ensure_block();
                IR::Block* return_block{block_pool.Create(inst_pool)};
                IR::IREmitter{*return_block}.Epilogue();
                current_block->AddBranch(return_block);

                auto& merge{syntax_list.emplace_back()};
                merge.type = IR::AbstractSyntaxNode::Type::Block;
                merge.data.block = return_block;

                current_block = nullptr;
                syntax_list.emplace_back().type = IR::AbstractSyntaxNode::Type::Return;
                break;
            }
            case StatementType::Unreachable: {
                ensure_block();
                current_block = nullptr;
                syntax_list.emplace_back().type = IR::AbstractSyntaxNode::Type::Unreachable;
                break;
            }
            default:
                UNREACHABLE_MSG("Statement type {}", u32(stmt.type));
            }
        }
        if (current_block) {
            if (fallthrough_block) {
                current_block->AddBranch(fallthrough_block);
            } else {
                syntax_list.emplace_back().type = IR::AbstractSyntaxNode::Type::Unreachable;
            }
        }
    }

    IR::Block* MergeBlock(Statement& parent, Statement& stmt) {
        Statement* merge_stmt{TryFindForwardBlock(stmt)};
        if (!merge_stmt) {
            // Create a merge block we can visit later
            merge_stmt = stmt_pool.Create(&dummy_flow_block, &parent);
            parent.children.insert(std::next(Tree::s_iterator_to(stmt)), *merge_stmt);
        }
        return block_pool.Create(inst_pool);
    }

    Common::ObjectPool<Statement>& stmt_pool;
    Common::ObjectPool<IR::Inst>& inst_pool;
    Common::ObjectPool<IR::Block>& block_pool;
    IR::AbstractSyntaxList& syntax_list;
    const Block dummy_flow_block{.is_dummy = true};
    std::span<const GcnInst> inst_list;
    Info& info;
    const RuntimeInfo& runtime_info;
    const Profile& profile;
    Translator translator;
};
} // Anonymous namespace

IR::AbstractSyntaxList BuildASL(Common::ObjectPool<IR::Inst>& inst_pool,
                                Common::ObjectPool<IR::Block>& block_pool, CFG& cfg, Info& info,
                                const RuntimeInfo& runtime_info, const Profile& profile) {
    Common::ObjectPool<Statement> stmt_pool{64};
    GotoPass goto_pass{cfg, stmt_pool};
    Statement& root{goto_pass.RootStatement()};
    IR::AbstractSyntaxList syntax_list;
    TranslatePass{inst_pool,     block_pool, stmt_pool,    root,   syntax_list,
                  cfg.inst_list, info,       runtime_info, profile};
    ASSERT_MSG(!info.translation_failed, "Shader translation has failed");
    return syntax_list;
}

} // namespace Shader::Gcn
