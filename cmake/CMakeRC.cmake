# This block is executed when generating an intermediate resource file, not when
# running in CMake configure mode
if(_CMRC_GENERATE_MODE)
    # Read in the digits
    file(READ "${INPUT_FILE}" bytes HEX)
    # Format each pair into a character literal. Heuristics seem to favor doing
    # the conversion in groups of five for fastest conversion
    string(REGEX REPLACE "(..)(..)(..)(..)(..)" "'\\\\x\\1','\\\\x\\2','\\\\x\\3','\\\\x\\4','\\\\x\\5'," chars "${bytes}")
    # Since we did this in groups, we have some leftovers to clean up
    string(LENGTH "${bytes}" n_bytes2)
    math(EXPR n_bytes "${n_bytes2} / 2")
    math(EXPR remainder "${n_bytes} % 5") # <-- '5' is the grouping count from above
    set(cleanup_re "$")
    set(cleanup_sub )
    while(remainder)
        set(cleanup_re "(..)${cleanup_re}")
        set(cleanup_sub "'\\\\x\\${remainder}',${cleanup_sub}")
        math(EXPR remainder "${remainder} - 1")
    endwhile()
    if(NOT cleanup_re STREQUAL "$")
        string(REGEX REPLACE "${cleanup_re}" "${cleanup_sub}" chars "${chars}")
    endif()
    string(CONFIGURE [[
        namespace { const char file_array[] = { @chars@ 0 }; }
        namespace cmrc { namespace @NAMESPACE@ { namespace res_chars {
        extern const char* const @SYMBOL@_begin = file_array;
        extern const char* const @SYMBOL@_end = file_array + @n_bytes@;
        }}}
    ]] code)
    file(WRITE "${OUTPUT_FILE}" "${code}")
    # Exit from the script. Nothing else needs to be processed
    return()
endif()

set(_version 2.0.0)

cmake_minimum_required(VERSION 3.12)
include(CMakeParseArguments)

if(COMMAND cmrc_add_resource_library)
    if(NOT DEFINED _CMRC_VERSION OR NOT (_version STREQUAL _CMRC_VERSION))
        message(WARNING "More than one CMakeRC version has been included in this project.")
    endif()
    # CMakeRC has already been included! Don't do anything
    return()
endif()

set(_CMRC_VERSION "${_version}" CACHE INTERNAL "CMakeRC version. Used for checking for conflicts")

set(_CMRC_SCRIPT "${CMAKE_CURRENT_LIST_FILE}" CACHE INTERNAL "Path to CMakeRC script")

function(_cmrc_normalize_path var)
    set(path "${${var}}")
    file(TO_CMAKE_PATH "${path}" path)
    while(path MATCHES "//")
        string(REPLACE "//" "/" path "${path}")
    endwhile()
    string(REGEX REPLACE "/+$" "" path "${path}")
    set("${var}" "${path}" PARENT_SCOPE)
endfunction()

get_filename_component(_inc_dir "${CMAKE_BINARY_DIR}/_cmrc/include" ABSOLUTE)
set(CMRC_INCLUDE_DIR "${_inc_dir}" CACHE INTERNAL "Directory for CMakeRC include files")
# Let's generate the primary include file
file(MAKE_DIRECTORY "${CMRC_INCLUDE_DIR}/cmrc")
set(hpp_content [==[
#ifndef CMRC_CMRC_HPP_INCLUDED
#define CMRC_CMRC_HPP_INCLUDED

#include <cassert>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <system_error>
#include <type_traits>

#if !(defined(__EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND) || defined(CMRC_NO_EXCEPTIONS))
#define CMRC_NO_EXCEPTIONS 1
#endif

namespace cmrc { namespace detail { struct dummy; } }

#define CMRC_DECLARE(libid) \
    namespace cmrc { namespace detail { \
    struct dummy; \
    static_assert(std::is_same<dummy, ::cmrc::detail::dummy>::value, "CMRC_DECLARE() must only appear at the global namespace"); \
    } } \
    namespace cmrc { namespace libid { \
    cmrc::embedded_filesystem get_filesystem(); \
    } } static_assert(true, "")

namespace cmrc {

class file {
    const char* _begin = nullptr;
    const char* _end = nullptr;

public:
    using iterator = const char*;
    using const_iterator = iterator;
    iterator begin() const noexcept { return _begin; }
    iterator cbegin() const noexcept { return _begin; }
    iterator end() const noexcept { return _end; }
    iterator cend() const noexcept { return _end; }
    std::size_t size() const { return static_cast<std::size_t>(std::distance(begin(), end())); }

    file() = default;
    file(iterator beg, iterator end) noexcept : _begin(beg), _end(end) {}
};

class directory_entry;

namespace detail {

class directory;
class file_data;

class file_or_directory {
    union _data_t {
        class file_data* file_data;
        class directory* directory;
    } _data;
    bool _is_file = true;

public:
    explicit file_or_directory(file_data& f) {
        _data.file_data = &f;
    }
    explicit file_or_directory(directory& d) {
        _data.directory = &d;
        _is_file = false;
    }
    bool is_file() const noexcept {
        return _is_file;
    }
    bool is_directory() const noexcept {
        return !is_file();
    }
    const directory& as_directory() const noexcept {
        assert(!is_file());
        return *_data.directory;
    }
    const file_data& as_file() const noexcept {
        assert(is_file());
        return *_data.file_data;
    }
};

class file_data {
public:
    const char* begin_ptr;
    const char* end_ptr;
    file_data(const file_data&) = delete;
    file_data(const char* b, const char* e) : begin_ptr(b), end_ptr(e) {}
};

inline std::pair<std::string, std::string> split_path(const std::string& path) {
    auto first_sep = path.find("/");
    if (first_sep == path.npos) {
        return std::make_pair(path, "");
    } else {
        return std::make_pair(path.substr(0, first_sep), path.substr(first_sep + 1));
    }
}

struct created_subdirectory {
    class directory& directory;
    class file_or_directory& index_entry;
};

class directory {
    std::list<file_data> _files;
    std::list<directory> _dirs;
    std::map<std::string, file_or_directory> _index;

    using base_iterator = std::map<std::string, file_or_directory>::const_iterator;

public:

    directory() = default;
    directory(const directory&) = delete;

    created_subdirectory add_subdir(std::string name) & {
        _dirs.emplace_back();
        auto& back = _dirs.back();
        auto& fod = _index.emplace(name, file_or_directory{back}).first->second;
        return created_subdirectory{back, fod};
    }

    file_or_directory* add_file(std::string name, const char* begin, const char* end) & {
        assert(_index.find(name) == _index.end());
        _files.emplace_back(begin, end);
        return &_index.emplace(name, file_or_directory{_files.back()}).first->second;
    }

    const file_or_directory* get(const std::string& path) const {
        auto pair = split_path(path);
        auto child = _index.find(pair.first);
        if (child == _index.end()) {
            return nullptr;
        }
        auto& entry  = child->second;
        if (pair.second.empty()) {
            // We're at the end of the path
            return &entry;
        }

        if (entry.is_file()) {
            // We can't traverse into a file. Stop.
            return nullptr;
        }
        // Keep going down
        return entry.as_directory().get(pair.second);
    }

    class iterator {
        base_iterator _base_iter;
        base_iterator _end_iter;
    public:
        using value_type = directory_entry;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::input_iterator_tag;

        iterator() = default;
        explicit iterator(base_iterator iter, base_iterator end) : _base_iter(iter), _end_iter(end) {}

        iterator begin() const noexcept {
            return *this;
        }

        iterator end() const noexcept {
            return iterator(_end_iter, _end_iter);
        }

        inline value_type operator*() const noexcept;

        bool operator==(const iterator& rhs) const noexcept {
            return _base_iter == rhs._base_iter;
        }

        bool operator!=(const iterator& rhs) const noexcept {
            return !(*this == rhs);
        }

        iterator& operator++() noexcept {
            ++_base_iter;
            return *this;
        }

        iterator operator++(int) noexcept {
            auto cp = *this;
            ++_base_iter;
            return cp;
        }
    };

    using const_iterator = iterator;

    iterator begin() const noexcept {
        return iterator(_index.begin(), _index.end());
    }

    iterator end() const noexcept {
        return iterator();
    }
};

inline std::string normalize_path(std::string path) {
    while (path.find("/") == 0) {
        path.erase(path.begin());
    }
    while (!path.empty() && (path.rfind("/") == path.size() - 1)) {
        path.pop_back();
    }
    auto off = path.npos;
    while ((off = path.find("//")) != path.npos) {
        path.erase(path.begin() + static_cast<std::string::difference_type>(off));
    }
    return path;
}

using index_type = std::map<std::string, const cmrc::detail::file_or_directory*>;

} // detail

class directory_entry {
    std::string _fname;
    const detail::file_or_directory* _item;

public:
    directory_entry() = delete;
    explicit directory_entry(std::string filename, const detail::file_or_directory& item)
        : _fname(filename)
        , _item(&item)
    {}

    const std::string& filename() const & {
        return _fname;
    }
    std::string filename() const && {
        return std::move(_fname);
    }

    bool is_file() const {
        return _item->is_file();
    }

    bool is_directory() const {
        return _item->is_directory();
    }
};

directory_entry detail::directory::iterator::operator*() const noexcept {
    assert(begin() != end());
    return directory_entry(_base_iter->first, _base_iter->second);
}

using directory_iterator = detail::directory::iterator;

class embedded_filesystem {
    // Never-null:
    const cmrc::detail::index_type* _index;
    const detail::file_or_directory* _get(std::string path) const {
        path = detail::normalize_path(path);
        auto found = _index->find(path);
        if (found == _index->end()) {
            return nullptr;
        } else {
            return found->second;
        }
    }

public:
    explicit embedded_filesystem(const detail::index_type& index)
        : _index(&index)
    {}

    file open(const std::string& path) const {
        auto entry_ptr = _get(path);
        if (!entry_ptr || !entry_ptr->is_file()) {
#ifdef CMRC_NO_EXCEPTIONS
            fprintf(stderr, "Error no such file or directory: %s\n", path.c_str());
            abort();
#else
            throw std::system_error(make_error_code(std::errc::no_such_file_or_directory), path);
#endif
        }
        auto& dat = entry_ptr->as_file();
        return file{dat.begin_ptr, dat.end_ptr};
    }

    bool is_file(const std::string& path) const noexcept {
        auto entry_ptr = _get(path);
        return entry_ptr && entry_ptr->is_file();
    }

    bool is_directory(const std::string& path) const noexcept {
        auto entry_ptr = _get(path);
        return entry_ptr && entry_ptr->is_directory();
    }

    bool exists(const std::string& path) const noexcept {
        return !!_get(path);
    }

    directory_iterator iterate_directory(const std::string& path) const {
        auto entry_ptr = _get(path);
        if (!entry_ptr) {
#ifdef CMRC_NO_EXCEPTIONS
            fprintf(stderr, "Error no such file or directory: %s\n", path.c_str());
            abort();
#else
            throw std::system_error(make_error_code(std::errc::no_such_file_or_directory), path);
#endif
        }
        if (!entry_ptr->is_directory()) {
#ifdef CMRC_NO_EXCEPTIONS
            fprintf(stderr, "Error not a directory: %s\n", path.c_str());
            abort();
#else
            throw std::system_error(make_error_code(std::errc::not_a_directory), path);
#endif
        }
        return entry_ptr->as_directory().begin();
    }
};

}

#endif // CMRC_CMRC_HPP_INCLUDED
]==])

set(cmrc_hpp "${CMRC_INCLUDE_DIR}/cmrc/cmrc.hpp" CACHE INTERNAL "")
set(_generate 1)
if(EXISTS "${cmrc_hpp}")
    file(READ "${cmrc_hpp}" _current)
    if(_current STREQUAL hpp_content)
        set(_generate 0)
    endif()
endif()
file(GENERATE OUTPUT "${cmrc_hpp}" CONTENT "${hpp_content}" CONDITION ${_generate})

add_library(cmrc-base INTERFACE)
target_include_directories(cmrc-base INTERFACE $<BUILD_INTERFACE:${CMRC_INCLUDE_DIR}>)
# Signal a basic C++11 feature to require C++11.
target_compile_features(cmrc-base INTERFACE cxx_nullptr)
set_property(TARGET cmrc-base PROPERTY INTERFACE_CXX_EXTENSIONS OFF)
add_library(cmrc::base ALIAS cmrc-base)

function(cmrc_add_resource_library name)
    set(args ALIAS NAMESPACE TYPE)
    cmake_parse_arguments(ARG "" "${args}" "" "${ARGN}")
    # Generate the identifier for the resource library's namespace
    set(ns_re "[a-zA-Z_][a-zA-Z0-9_]*")
    if(NOT DEFINED ARG_NAMESPACE)
        # Check that the library name is also a valid namespace
        if(NOT name MATCHES "${ns_re}")
            message(SEND_ERROR "Library name is not a valid namespace. Specify the NAMESPACE argument")
        endif()
        set(ARG_NAMESPACE "${name}")
    else()
        if(NOT ARG_NAMESPACE MATCHES "${ns_re}")
            message(SEND_ERROR "NAMESPACE for ${name} is not a valid C++ namespace identifier (${ARG_NAMESPACE})")
        endif()
    endif()
    set(libname "${name}")
    # Check that type is either "STATIC" or "OBJECT", or default to "STATIC" if
    # not set
    if(NOT DEFINED ARG_TYPE)
        set(ARG_TYPE STATIC)
    elseif(NOT "${ARG_TYPE}" MATCHES "^(STATIC|OBJECT)$")
        message(SEND_ERROR "${ARG_TYPE} is not a valid TYPE (STATIC and OBJECT are acceptable)")
        set(ARG_TYPE STATIC)
    endif()
    # Generate a library with the compiled in character arrays.
    string(CONFIGURE [=[
        #include <cmrc/cmrc.hpp>
        #include <map>
        #include <utility>

        namespace cmrc {
        namespace @ARG_NAMESPACE@ {

        namespace res_chars {
        // These are the files which are available in this resource library
        $<JOIN:$<TARGET_PROPERTY:@libname@,CMRC_EXTERN_DECLS>,
        >
        }

        namespace {

        const cmrc::detail::index_type&
        get_root_index() {
            static cmrc::detail::directory root_directory_;
            static cmrc::detail::file_or_directory root_directory_fod{root_directory_};
            static cmrc::detail::index_type root_index;
            root_index.emplace("", &root_directory_fod);
            struct dir_inl {
                class cmrc::detail::directory& directory;
            };
            dir_inl root_directory_dir{root_directory_};
            (void)root_directory_dir;
            $<JOIN:$<TARGET_PROPERTY:@libname@,CMRC_MAKE_DIRS>,
            >
            $<JOIN:$<TARGET_PROPERTY:@libname@,CMRC_MAKE_FILES>,
            >
            return root_index;
        }

        }

        cmrc::embedded_filesystem get_filesystem() {
            static auto& index = get_root_index();
            return cmrc::embedded_filesystem{index};
        }

        } // @ARG_NAMESPACE@
        } // cmrc
    ]=] cpp_content @ONLY)
    get_filename_component(libdir "${CMAKE_CURRENT_BINARY_DIR}/__cmrc_${name}" ABSOLUTE)
    get_filename_component(lib_tmp_cpp "${libdir}/lib_.cpp" ABSOLUTE)
    string(REPLACE "\n        " "\n" cpp_content "${cpp_content}")
    file(GENERATE OUTPUT "${lib_tmp_cpp}" CONTENT "${cpp_content}")
    get_filename_component(libcpp "${libdir}/lib.cpp" ABSOLUTE)
    add_custom_command(OUTPUT "${libcpp}"
        DEPENDS "${lib_tmp_cpp}" "${cmrc_hpp}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${lib_tmp_cpp}" "${libcpp}"
        COMMENT "Generating ${name} resource loader"
        )
    # Generate the actual static library. Each source file is just a single file
    # with a character array compiled in containing the contents of the
    # corresponding resource file.
    add_library(${name} ${ARG_TYPE} ${libcpp})
    set_property(TARGET ${name} PROPERTY CMRC_LIBDIR "${libdir}")
    set_property(TARGET ${name} PROPERTY CMRC_NAMESPACE "${ARG_NAMESPACE}")
    target_link_libraries(${name} PUBLIC cmrc::base)
    set_property(TARGET ${name} PROPERTY CMRC_IS_RESOURCE_LIBRARY TRUE)
    if(ARG_ALIAS)
        add_library("${ARG_ALIAS}" ALIAS ${name})
    endif()
    cmrc_add_resources(${name} ${ARG_UNPARSED_ARGUMENTS})
endfunction()

function(_cmrc_register_dirs name dirpath)
    if(dirpath STREQUAL "")
        return()
    endif()
    # Skip this dir if we have already registered it
    get_target_property(registered "${name}" _CMRC_REGISTERED_DIRS)
    if(dirpath IN_LIST registered)
        return()
    endif()
    # Register the parent directory first
    get_filename_component(parent "${dirpath}" DIRECTORY)
    if(NOT parent STREQUAL "")
        _cmrc_register_dirs("${name}" "${parent}")
    endif()
    # Now generate the registration
    set_property(TARGET "${name}" APPEND PROPERTY _CMRC_REGISTERED_DIRS "${dirpath}")
    _cm_encode_fpath(sym "${dirpath}")
    if(parent STREQUAL "")
        set(parent_sym root_directory)
    else()
        _cm_encode_fpath(parent_sym "${parent}")
    endif()
    get_filename_component(leaf "${dirpath}" NAME)
    set_property(
        TARGET "${name}"
        APPEND PROPERTY CMRC_MAKE_DIRS
        "static auto ${sym}_dir = ${parent_sym}_dir.directory.add_subdir(\"${leaf}\")\;"
        "root_index.emplace(\"${dirpath}\", &${sym}_dir.index_entry)\;"
        )
endfunction()

function(cmrc_add_resources name)
    get_target_property(is_reslib ${name} CMRC_IS_RESOURCE_LIBRARY)
    if(NOT TARGET ${name} OR NOT is_reslib)
        message(SEND_ERROR "cmrc_add_resources called on target '${name}' which is not an existing resource library")
        return()
    endif()

    set(options)
    set(args WHENCE PREFIX)
    set(list_args)
    cmake_parse_arguments(ARG "${options}" "${args}" "${list_args}" "${ARGN}")

    if(NOT ARG_WHENCE)
        set(ARG_WHENCE ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    _cmrc_normalize_path(ARG_WHENCE)
    get_filename_component(ARG_WHENCE "${ARG_WHENCE}" ABSOLUTE)

    # Generate the identifier for the resource library's namespace
    get_target_property(lib_ns "${name}" CMRC_NAMESPACE)

    get_target_property(libdir ${name} CMRC_LIBDIR)
    get_target_property(target_dir ${name} SOURCE_DIR)
    file(RELATIVE_PATH reldir "${target_dir}" "${CMAKE_CURRENT_SOURCE_DIR}")
    if(reldir MATCHES "^\\.\\.")
        message(SEND_ERROR "Cannot call cmrc_add_resources in a parent directory from the resource library target")
        return()
    endif()

    foreach(input IN LISTS ARG_UNPARSED_ARGUMENTS)
        _cmrc_normalize_path(input)
        get_filename_component(abs_in "${input}" ABSOLUTE)
        # Generate a filename based on the input filename that we can put in
        # the intermediate directory.
        file(RELATIVE_PATH relpath "${ARG_WHENCE}" "${abs_in}")
        if(relpath MATCHES "^\\.\\.")
            # For now we just error on files that exist outside of the soure dir.
            message(SEND_ERROR "Cannot add file '${input}': File must be in a subdirectory of ${ARG_WHENCE}")
            continue()
        endif()
        if(DEFINED ARG_PREFIX)
            _cmrc_normalize_path(ARG_PREFIX)
        endif()
        if(ARG_PREFIX AND NOT ARG_PREFIX MATCHES "/$")
            set(ARG_PREFIX "${ARG_PREFIX}/")
        endif()
        get_filename_component(dirpath "${ARG_PREFIX}${relpath}" DIRECTORY)
        _cmrc_register_dirs("${name}" "${dirpath}")
        get_filename_component(abs_out "${libdir}/intermediate/${ARG_PREFIX}${relpath}.cpp" ABSOLUTE)
        # Generate a symbol name relpath the file's character array
        _cm_encode_fpath(sym "${relpath}")
        # Get the symbol name for the parent directory
        if(dirpath STREQUAL "")
            set(parent_sym root_directory)
        else()
            _cm_encode_fpath(parent_sym "${dirpath}")
        endif()
        # Generate the rule for the intermediate source file
        _cmrc_generate_intermediate_cpp(${lib_ns} ${sym} "${abs_out}" "${abs_in}")
        target_sources(${name} PRIVATE "${abs_out}")
        set_property(TARGET ${name} APPEND PROPERTY CMRC_EXTERN_DECLS
            "// Pointers to ${input}"
            "extern const char* const ${sym}_begin\;"
            "extern const char* const ${sym}_end\;"
            )
        get_filename_component(leaf "${relpath}" NAME)
        set_property(
            TARGET ${name}
            APPEND PROPERTY CMRC_MAKE_FILES
            "root_index.emplace("
            "    \"${ARG_PREFIX}${relpath}\","
            "    ${parent_sym}_dir.directory.add_file("
            "        \"${leaf}\","
            "        res_chars::${sym}_begin,"
            "        res_chars::${sym}_end"
            "    )"
            ")\;"
            )
    endforeach()
endfunction()

function(_cmrc_generate_intermediate_cpp lib_ns symbol outfile infile)
    add_custom_command(
        # This is the file we will generate
        OUTPUT "${outfile}"
        # These are the primary files that affect the output
        DEPENDS "${infile}" "${_CMRC_SCRIPT}"
        COMMAND
            "${CMAKE_COMMAND}"
                -D_CMRC_GENERATE_MODE=TRUE
                -DNAMESPACE=${lib_ns}
                -DSYMBOL=${symbol}
                "-DINPUT_FILE=${infile}"
                "-DOUTPUT_FILE=${outfile}"
                -P "${_CMRC_SCRIPT}"
        COMMENT "Generating intermediate file for ${infile}"
    )
endfunction()

function(_cm_encode_fpath var fpath)
    string(MAKE_C_IDENTIFIER "${fpath}" ident)
    string(MD5 hash "${fpath}")
    string(SUBSTRING "${hash}" 0 4 hash)
    set(${var} f_${hash}_${ident} PARENT_SCOPE)
endfunction()
