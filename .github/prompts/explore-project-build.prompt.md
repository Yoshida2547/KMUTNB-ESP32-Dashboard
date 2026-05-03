---
description: "Explore the project and find the build command"
name: "Explore Project Build"
agent: "agent"
argument-hint: "Inspect the project and determine how to build it"
---
Inspect the current workspace and determine how to run a common build task for this project.

Focus on:
- Identifying the build system, framework, and any project-specific tooling
- Finding the exact command to build the project from the repository root
- Noting any prerequisites, environment variables, or setup steps required before building
- Calling out any alternative build variants if they exist, such as debug or release configurations

When you respond, keep it practical:
- State the build command first
- Mention where you found it in the repo
- Include only the minimum setup details needed to run it successfully
- If the build command is ambiguous, explain the evidence and the safest likely command to try next
