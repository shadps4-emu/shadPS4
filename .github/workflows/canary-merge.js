// SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Canary merge bot, run from canary.yml through actions/github-script.
// Based on citra's ci-merge.js.

const fs = require("fs");

const LABEL = process.env.CANARY_LABEL || "canary";
const TARGET_REPO = process.env.CANARY_REPO || "shadps4-emu/shadPS4-canary";
const REQUIRE_APPROVAL = (process.env.CANARY_REQUIRE_APPROVAL || "true") !== "false";
const MAINTAINER_ROLES = ["OWNER", "MEMBER", "COLLABORATOR"];

async function git(exec, args, opts = {}) {
    const result = await exec.getExecOutput("git", args, { silent: true, ...opts });
    return result;
}

async function gitOk(exec, args) {
    const result = await git(exec, args, { ignoreReturnCode: true });
    return result.exitCode === 0;
}

async function getLabelledPulls(github, context) {
    const query = `query($owner: String!, $name: String!, $label: String!, $cursor: String) {
        repository(owner: $owner, name: $name) {
            pullRequests(labels: [$label], states: OPEN, first: 50, after: $cursor,
                         orderBy: {field: CREATED_AT, direction: ASC}) {
                pageInfo { hasNextPage endCursor }
                nodes {
                    number title isDraft headRefOid
                    author { login }
                    reviews(states: APPROVED, last: 50) {
                        nodes { authorAssociation commit { oid } submittedAt }
                    }
                }
            }
        }
    }`;
    let pulls = [];
    let cursor = null;
    do {
        const result = await github.graphql(query, {
            owner: context.repo.owner,
            name: context.repo.repo,
            label: LABEL,
            cursor,
        });
        const page = result.repository.pullRequests;
        pulls = pulls.concat(page.nodes);
        cursor = page.pageInfo.hasNextPage ? page.pageInfo.endCursor : null;
    } while (cursor);
    return pulls;
}

// The commit of this PR to merge, or null (with a reason) to skip it.
function pickCommit(pull) {
    if (pull.isDraft) {
        return { sha: null, reason: "draft" };
    }
    if (!REQUIRE_APPROVAL) {
        return { sha: pull.headRefOid, reason: null };
    }
    const approvals = pull.reviews.nodes
        .filter((r) => r.commit && MAINTAINER_ROLES.includes(r.authorAssociation))
        .sort((a, b) => new Date(a.submittedAt) - new Date(b.submittedAt));
    if (approvals.length === 0) {
        return { sha: null, reason: "no maintainer approval" };
    }
    return { sha: approvals[approvals.length - 1].commit.oid, reason: null };
}

async function fetchCommit(exec, repoUrl, number, sha) {
    // The PR head usually contains the approved commit; if it was force-pushed
    // away, GitHub still serves the commit by its id.
    await git(exec, ["fetch", "-q", "--no-recurse-submodules", repoUrl, `pull/${number}/head`], {
        ignoreReturnCode: true,
    });
    if (await gitOk(exec, ["cat-file", "-e", `${sha}^{commit}`])) {
        return true;
    }
    await git(exec, ["fetch", "-q", "--no-recurse-submodules", repoUrl, sha], {
        ignoreReturnCode: true,
    });
    return gitOk(exec, ["cat-file", "-e", `${sha}^{commit}`]);
}

// True if merging `sha` into `onto` would conflict. Uses merge-tree, so the
// work tree and index are left alone. Needs git 2.38+.
async function conflicts(exec, onto, sha) {
    const r = await git(exec, ["merge-tree", "--write-tree", "--no-messages", onto, sha], {
        ignoreReturnCode: true,
    });
    if (r.exitCode > 1) {
        throw new Error(`git merge-tree failed: ${r.stderr}`);
    }
    return r.exitCode === 1;
}

// A commit holding `sha` squash-merged onto `base`, or null if that conflicts.
async function squashOnto(exec, base, sha) {
    const r = await git(exec, ["merge-tree", "--write-tree", "--no-messages", base, sha], {
        ignoreReturnCode: true,
    });
    if (r.exitCode !== 0) {
        return null;
    }
    const tree = r.stdout.split("\n")[0].trim();
    return (await git(exec, ["commit-tree", tree, "-p", base, "-m", "probe"])).stdout.trim();
}

// Why `sha` could not be merged: it conflicts with main itself, with one or
// more earlier canary PRs on their own, or only with their combination.
async function explainConflict(exec, base, sha, mergedBefore) {
    if (await conflicts(exec, base, sha)) {
        return "conflicts with main";
    }
    const culprits = [];
    for (const other of mergedBefore) {
        const probe = await squashOnto(exec, base, other.sha);
        if (probe && (await conflicts(exec, probe, sha))) {
            culprits.push(`#${other.pull.number}`);
        }
    }
    if (culprits.length > 0) {
        return `conflicts with ${culprits.join(", ")}`;
    }
    return "conflicts with the combination of earlier canary PRs";
}

async function lastCanary(exec, targetUrl) {
    // Latest canary number from the target's tags, and the inputs of its last build.
    const tags = await git(exec, ["ls-remote", "--tags", "--refs", targetUrl, "refs/tags/canary-*"], {
        ignoreReturnCode: true,
    });
    let number = 0;
    for (const line of tags.stdout.split("\n")) {
        const m = /refs\/tags\/canary-(\d+)$/.exec(line.trim());
        if (m) {
            number = Math.max(number, parseInt(m[1], 10));
        }
    }
    let inputs = "";
    if (await gitOk(exec, ["fetch", "-q", "--depth", "1", targetUrl, "refs/heads/main:refs/canary/previous"])) {
        const body = await git(exec, ["log", "-1", "--format=%B", "refs/canary/previous"]);
        const m = /^Canary-Inputs: (.*)$/m.exec(body.stdout);
        inputs = m ? m[1].trim() : "";
    }
    return { number, inputs };
}

function mergeLog(results, repoUrl) {
    let out = "| Pull Request | Commit | Title | Author | Merged? |\n|----|----|----|----|----|\n";
    for (const r of results) {
        const pr = r.pull.number;
        const title = r.pull.title.replace(/\|/g, "\\|");
        const author = r.pull.author ? r.pull.author.login : "ghost";
        const commit = r.sha ? `[\`${r.sha.slice(0, 9)}\`](${repoUrl}/pull/${pr}/commits/${r.sha})` : "N/A";
        const status = r.merged ? "Yes" : `No (${r.reason})`;
        out += `| [#${pr}](${repoUrl}/pull/${pr}) | ${commit} | ${title} | [${author}](https://github.com/${author}) | ${status} |\n`;
    }
    return out;
}

async function mergebot({ github, context, exec, core }) {
    const force = (process.env.CANARY_FORCE || "false") === "true";
    const token = process.env.CANARY_PUSH_TOKEN;
    if (!token) {
        core.setFailed(`Set the CANARY_PUSH_TOKEN secret: a token with contents write access to ${TARGET_REPO}.`);
        return;
    }
    const repoUrl = `https://github.com/${context.repo.owner}/${context.repo.repo}`;
    const targetUrl = `https://x-access-token:${token}@github.com/${TARGET_REPO}.git`;
    core.setSecret(token);

    await git(exec, ["config", "user.name", "shadPS4 Canary Bot"]);
    await git(exec, ["config", "user.email", "41898282+github-actions[bot]@users.noreply.github.com"]);

    const base = (await git(exec, ["rev-parse", "HEAD"])).stdout.trim();
    const pulls = await getLabelledPulls(github, context);

    core.startGroup("Pick pull requests");
    const picks = [];
    for (const pull of pulls) {
        const { sha, reason } = pickCommit(pull);
        picks.push({ pull, sha, reason, merged: false });
        core.info(`#${pull.number} ${pull.title}: ${sha ? sha : "skipped (" + reason + ")"}`);
    }
    core.endGroup();

    const inputs = [base.slice(0, 12)]
        .concat(picks.filter((p) => p.sha).map((p) => `${p.pull.number}@${p.sha.slice(0, 12)}`))
        .join(" ");
    const previous = await lastCanary(exec, targetUrl);
    if (!force && previous.inputs === inputs) {
        core.notice(`Nothing changed since canary-${previous.number} (${inputs}); skipping.`);
        return;
    }

    core.startGroup("Merge pull requests");
    for (const p of picks) {
        if (!p.sha) {
            continue;
        }
        const pr = p.pull.number;
        if (!(await fetchCommit(exec, repoUrl, pr, p.sha))) {
            p.reason = "commit not found";
            core.warning(`#${pr}: could not fetch ${p.sha}`);
            continue;
        }
        const merge = await git(exec, ["merge", "--squash", "--no-edit", p.sha], { ignoreReturnCode: true });
        if (merge.exitCode !== 0) {
            await git(exec, ["reset", "-q", "--hard", "HEAD"]);
            const merged = picks.filter((q) => q.merged);
            p.reason = await explainConflict(exec, base, p.sha, merged);
            core.warning(`#${pr} not merged: ${p.reason}`);
            continue;
        }
        if (await gitOk(exec, ["diff", "--cached", "--quiet"])) {
            p.reason = "no changes";
            continue;
        }
        await git(exec, ["commit", "-q", "-m", `Merge PR #${pr}: ${p.pull.title}`]);
        p.merged = true;
        core.info(`#${pr} merged at ${p.sha.slice(0, 9)}`);
    }
    core.endGroup();

    const number = previous.number + 1;
    const log = mergeLog(picks, repoUrl);
    const readme = fs.existsSync("README.md") ? fs.readFileSync("README.md", "utf8") : "";
    fs.writeFileSync(
        "README.md",
        `# shadPS4 Canary #${number}\n\n` +
            `Built from [main@${base.slice(0, 9)}](${repoUrl}/commit/${base}) with these pull requests:\n\n` +
            log +
            "\n\nEnd of merge log. The original README.md follows.\n\n-----\n\n" +
            readme
    );
    await git(exec, ["add", "README.md"]);
    await git(exec, [
        "commit", "-q",
        "-m", `Canary #${number}`,
        "-m", log,
        "-m", `Canary-Inputs: ${inputs}`,
    ]);

    const tag = `canary-${number}`;
    await git(exec, ["tag", tag]);
    core.info(`Pushing ${tag} to ${TARGET_REPO} ...`);
    // Tag first: the branch push starts the canary build, which looks the tag up.
    await git(exec, ["push", "-q", targetUrl, `refs/tags/${tag}`]);
    await git(exec, ["push", "-q", "--force", targetUrl, "HEAD:refs/heads/main"]);

    await core.summary.addHeading(`Canary #${number}`).addRaw(log, true).write();
    core.notice(`Pushed ${tag}: ${picks.filter((p) => p.merged).length} of ${picks.length} labelled PRs merged.`);
}

module.exports = { mergebot };
