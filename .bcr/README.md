# Bazel Central Registry (BCR) Publishing

This directory contains configuration files for automatically publishing `fast_float` releases to the [Bazel Central Registry (BCR)](https://github.com/bazelbuild/bazel-central-registry).

## Overview

When a new release is published on GitHub, the workflow defined in `.github/workflows/publish-to-bcr.yml` automatically:
1. Creates a new entry in the BCR with the release version
2. Opens a pull request to the [bazel-central-registry](https://github.com/bazelbuild/bazel-central-registry) repository
3. Allows BCR maintainers to review and merge the update

This makes the new version of `fast_float` available to all Bazel users through the standard module system.

## Files in This Directory

### `metadata.template.json`
Contains repository metadata including:
- Project homepage
- Maintainer information (name, email, GitHub username)
- Repository references

This information is used to populate the BCR entry's `metadata.json` file.

### `source.template.json`
Defines how to fetch the source code for each release:
- URL pattern for release archives
- Archive strip prefix configuration
- Integrity hash (auto-generated during publishing)

### `presubmit.yml`
Specifies the CI tests that BCR will run before accepting a new version:
- Platforms to test on (Linux, macOS, Windows)
- Bazel versions to test with
- Build targets to verify

## How the Publishing Workflow Works

The publishing process is automated through GitHub Actions:

1. **Trigger**: When you create a new release on GitHub (or manually trigger the workflow)
2. **Build**: The workflow uses [bazel-contrib/publish-to-bcr](https://github.com/bazel-contrib/publish-to-bcr) reusable workflow
3. **Generate**: Creates BCR entry files from the templates in this directory
4. **Submit**: Opens a pull request to the Bazel Central Registry
5. **Review**: BCR maintainers review and merge the PR

## Setting Up the Personal Access Token (PAT)

The publishing workflow requires a GitHub Personal Access Token with appropriate permissions to:
- Push to a fork of the Bazel Central Registry
- Open pull requests against the upstream BCR

### Creating a Personal Access Token

1. **Navigate to GitHub Settings**
   - Go to https://github.com/settings/tokens
   - Or: Click your profile picture → Settings → Developer settings → Personal access tokens → Tokens (classic)

2. **Generate New Token**
   - Click "Generate new token (classic)"
   - Give it a descriptive name, e.g., "BCR Publishing for fast_float"
   - Set an appropriate expiration (recommended: 1 year with calendar reminder to renew)

3. **Select Required Scopes**
   - ✅ **`repo`** (Full control of private repositories) - Required for accessing repository details
   - ✅ **`workflow`** (Update GitHub Action workflows) - Required for the publishing action

4. **Generate and Copy**
   - Click "Generate token" at the bottom
   - **Important**: Copy the token immediately - you won't be able to see it again!

5. **Add as Repository Secret**
   - Go to the fast_float repository settings
   - Navigate to: Settings → Secrets and variables → Actions
   - Click "New repository secret"
   - Name: `BCR_PUBLISH_TOKEN`
   - Value: Paste the token you copied
   - Click "Add secret"

### Important Notes

- **Classic PATs vs Fine-grained PATs**: Currently, you must use a "Classic" PAT. Fine-grained PATs don't yet support opening pull requests against public repositories (though GitHub is working on this).
  
- **Token Security**: Keep the token secure and never commit it to the repository. GitHub Secrets are encrypted and only exposed to authorized workflows.

- **Token Expiration**: Set a calendar reminder before your token expires to generate a new one, otherwise the publishing workflow will fail.

- **Permissions Required**: The token needs both `repo` and `workflow` scopes to function properly.

## Fork Requirements

The publishing workflow requires a fork of the [bazel-central-registry](https://github.com/bazelbuild/bazel-central-registry) repository:

- The fork should be in the repository owner's account or organization
- The PAT must have write access to this fork
- The workflow will push changes to the fork and open PRs from there to the upstream BCR

## Manual Triggering

You can manually trigger the publishing workflow:

1. Go to the Actions tab in the GitHub repository
2. Select "Publish to BCR" workflow
3. Click "Run workflow"
4. Enter the tag name (e.g., `v6.1.6`)
5. Click "Run workflow"

This is useful for:
- Republishing a release if the automatic workflow failed
- Publishing an older release that wasn't automatically published
- Testing the workflow

## Troubleshooting

### Publishing Workflow Fails

1. **Check the workflow logs** in the Actions tab for specific error messages
2. **Verify the PAT** is still valid and has the correct permissions
3. **Confirm the fork exists** and the PAT has access to it
4. **Check the template files** in this directory are valid JSON

### Pull Request Not Opening

1. **Verify PAT permissions** - must include `repo` and `workflow` scopes
2. **Check fork configuration** - ensure the BCR fork exists and is accessible
3. **Review workflow inputs** - ensure the tag name matches the actual release tag

### BCR Presubmit Tests Fail

1. **Review presubmit.yml** to ensure test configuration is correct
2. **Check build targets** are valid in the new version
3. **Verify MODULE.bazel** and BUILD files are properly configured
4. The BCR maintainers may provide feedback on the pull request

## Updating Configuration

### To Update Maintainer Information

Edit `metadata.template.json` and update the maintainers array.

### To Change Test Configuration

Edit `presubmit.yml` to modify:
- Platforms to test on
- Bazel versions to test with
- Build targets to verify

### To Modify Source Archive Settings

Edit `source.template.json` if the release archive structure changes.

## Resources

- [Bazel Central Registry](https://github.com/bazelbuild/bazel-central-registry)
- [publish-to-bcr Documentation](https://github.com/bazel-contrib/publish-to-bcr)
- [Bzlmod User Guide](https://bazel.build/docs/bzlmod)
- [GitHub PAT Documentation](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens)

## Support

If you encounter issues with BCR publishing:
1. Check the [publish-to-bcr issues](https://github.com/bazel-contrib/publish-to-bcr/issues)
2. Review [BCR submission guidelines](https://github.com/bazelbuild/bazel-central-registry/blob/main/docs/README.md)
3. Open an issue in the fast_float repository for project-specific problems
