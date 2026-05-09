--
Skill: Git Commit & Push
Related skill: agent-customization
Scope: workspace
--

Purpose
-------
Provide a standardized, reproducible workflow for committing and pushing changes to the GitHub repository after any code modifications. This skill enforces scope-based commit messages and ensures all work is persisted to the remote repository before task completion.

When to use
-----------
- MANDATORY: Whenever any file changes are made during development (required before agent task completion).
- After sensor implementations, UI changes, build fixes, documentation updates.
- After running calibration utilities that modify configuration or data files.

Step-by-step Workflow
---------------------

1. Verify changes exist
   - List all modified, staged, and untracked files:

     ```bash
     git status
     ```

   - If no changes are reported, the task is complete (no commit needed).
   - If changes exist, proceed to step 2.

2. Stage changes
   - Stage all modified files (recommended for focused work):

     ```bash
     git add .
     ```

   - Or stage specific files if you want fine-grained control:

     ```bash
     git add src/MyFile.cpp include/MyHeader.h
     ```

3. Compose commit message
   - Use scope-based format (enforced in this project):

     ```
     <scope>(<component>): <description>
     ```

   - Format rules:
     - `<scope>`: Use one of: `fix`, `feat`, `refactor`, `docs`, `build`, `test`, `perf`, `chore`
     - `<component>`: Affected area (e.g., `display`, `sensor`, `ui`, `manager`, `build`)
     - `<description>`: Concise, lowercase, imperative mood (e.g., "add touch calibration" not "Added touch calibration")
     - Total length: 50 characters or less (for readability in git log)

   - Examples:
     ```
     fix(display): correct touch coordinate mapping
     feat(sensor): add HDC1080 temperature sensor support
     refactor(manager): improve thread safety in SensorManager
     docs: update README with hardware configuration
     build(config): adjust LVGL buffer size for ILI9488
     ```

4. Commit changes
   - Use the prepared message:

     ```bash
     git commit -m "fix(display): correct touch coordinate mapping"
     ```

   - Verify commit succeeded (exit code 0):
     ```bash
     git log -1 --oneline
     ```

5. Push to remote
   - Push to master (upstream auto-set after first push):

     ```bash
     git push origin master
     ```

   - Or use `-u` flag on first push to set upstream:

     ```bash
     git push -u origin master
     ```

   - Verify push succeeded:
     ```bash
     git status
     ```
     Expected: "Your branch is up to date with 'origin/master'."

Decision Points & Branching
---------------------------
- **No changes detected**: Exit gracefully; no commit needed.
- **Conflicts on push**: Pull latest changes first:
  ```bash
  git pull origin master
  ```
  Then resolve conflicts and retry push.

- **Push rejected**: Verify you're on `master` branch:
  ```bash
  git branch
  git checkout master
  ```
  Then retry.

- **Uncommitted changes after pull**: Stage and commit again:
  ```bash
  git add .
  git commit -m "scope(component): description"
  git push origin master
  ```

Quality Criteria / Completion Checks
------------------------------------
- `git status` shows "On branch master" and "nothing to commit, working tree clean".
- `git log -1 --oneline` displays the commit with correct scope/component format.
- Push completes with exit code 0 (no network errors).
- Remote GitHub repository reflects the new commit within 1 minute.
- Commit message follows scope-based format and is ≤50 characters.

Troubleshooting Tips
--------------------
- **"Permission denied" on push**: Verify SSH key is loaded:
  ```bash
  ssh -T git@github.com
  ```
  If fails, add key: `ssh-add ~/.ssh/id_rsa`

- **"Detached HEAD" state**: Reattach to master:
  ```bash
  git checkout master
  ```

- **"Branch diverged"**: Pull and rebase:
  ```bash
  git pull --rebase origin master
  ```

- **Accidentally pushed to wrong branch**: Contact repo admin; do not force-push in this project.

- **Commit message format wrong**: Amend the latest commit:
  ```bash
  git commit --amend -m "fix(display): correct message"
  git push origin master --force-with-lease
  ```
  (Use sparingly; only for local commits not yet pulled by others)

Examples of Useful Prompts For Automation
---------------------------------------
- "After implementing the sensor driver, commit with message 'feat(sensor): add HDC1080 temperature sensor' and push."
- "Save calibration changes and commit with 'fix(display): update touch calibration data'."
- "Commit all documentation updates with 'docs: add sensor integration guide' and push to master."
- "After refactoring SensorManager, commit with 'refactor(manager): improve thread safety' and push."

How to Iterate
--------------
1. Make focused code changes (single feature or fix per commit).
2. Use `git status` to review what changed.
3. Stage changes with `git add .`
4. Compose a clear scope-based commit message.
5. Commit and push using steps 4-5 above.
6. Verify `git status` shows working tree clean and branch up-to-date.
7. Repeat for next change.

Important: Mandatory Workflow
-----------------------------
Per `.github/copilot-instructions.md`:
> "Any agent that changes files in this project must create a commit for those changes before finishing."

This is NOT optional. Every agent task that modifies files must:
1. Commit changes with scope-based message before task completion.
2. Push to `origin master` to persist changes.
3. Verify push succeeded and working tree is clean.

Failure to commit/push is considered an incomplete task.

Notes for Agents
----------------
- This skill is workspace-scoped and mandatory for all agent tasks.
- Always check `git status` before starting to understand the baseline.
- Stage ALL modified files in one `git add .` (unless explicitly selective).
- Commit message scope and component must align with actual changes (verify before committing).
- Never force-push; use `--force-with-lease` only in emergencies for amending local commits.
- If unsure about commit message, err on the side of being descriptive (e.g., `fix(sensor): correct i2c initialization timing after power loss`).
- After push, wait 1-2 seconds and run `git status` to confirm upstream sync.

Scope Reference (From Project Conventions)
-------------------------------------------
- **fix**: Bug fixes or correctness improvements
  - Example: "fix(display): correct touch coordinate mapping"
  - Use when: Correcting erroneous behavior, race conditions, data corruption

- **feat**: New feature or functionality
  - Example: "feat(sensor): add HDC1080 temperature sensor support"
  - Use when: Adding new sensor, screen, or user-facing capability

- **refactor**: Code restructuring without functional change
  - Example: "refactor(manager): improve thread safety in SensorManager"
  - Use when: Improving code quality, design patterns, performance (no behavior change)

- **docs**: Documentation-only changes
  - Example: "docs: add sensor integration guide to README"
  - Use when: Adding/updating README, comments, docstrings, guides

- **build**: Build system, dependencies, or configuration
  - Example: "build(config): adjust LVGL buffer size for ILI9488"
  - Use when: Modifying platformio.ini, scripts, build flags

- **test**: Test-related changes
  - Example: "test(i2c_scanner): add device address logging"
  - Use when: Adding/modifying test environments or utilities

- **perf**: Performance improvements
  - Example: "perf(ui): optimize screen rendering loop"
  - Use when: Reducing latency, memory, or CPU usage (measure before/after)

- **chore**: Maintenance, cleanup, no functional impact
  - Example: "chore: remove unused imports"
  - Use when: Code cleanup, dependency updates, formatting

Suggested follow-ups / Next skills
----------------------------------
- **Skill: Build-Upload-Monitor** – After commit/push, build and verify firmware changes on device.
- **Skill: Code Review** – Before commit, use the `.code-review` agent to validate design patterns and thread safety.
- **Skill: Git Workflow Advanced** – Branch management, rebase, cherry-pick for complex scenarios (future).

---

**Last Updated**: May 9, 2026 | **Status**: ✅ Active (mandatory for all agents) | **Version**: 1.0
