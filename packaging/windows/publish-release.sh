#!/usr/bin/env bash
set -euo pipefail

tag=${1:?version tag required}
assets=${2:?artifact directory required}
[[ "$tag" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo 'Invalid version tag' >&2; exit 1; }
cd "$assets"
sha256sum --check SHA256SUMS.txt

# A retry verifies published bytes rather than replacing an existing release.
if gh release view "$tag" --json isDraft >/dev/null 2>&1; then
  draft=$(gh release view "$tag" --json isDraft --jq .isDraft)
  if [[ "$draft" == false ]]; then
    check_dir=$(mktemp -d)
    trap 'rm -rf "$check_dir"' EXIT
    gh release download "$tag" --pattern Lantern-Windows-x64.zip --pattern SHA256SUMS.txt --dir "$check_dir"
    cmp SHA256SUMS.txt "$check_dir/SHA256SUMS.txt"
    (cd "$check_dir" && sha256sum --check SHA256SUMS.txt)
    echo "Published $tag already matches the tested artifacts"
    exit 0
  fi
else
  gh release create "$tag" --verify-tag --draft --title "Lantern for Windows $tag" --generate-notes
fi

gh release upload "$tag" Lantern-Windows-x64.zip SHA256SUMS.txt --clobber
check_dir=$(mktemp -d)
trap 'rm -rf "$check_dir"' EXIT
gh release download "$tag" --pattern Lantern-Windows-x64.zip --pattern SHA256SUMS.txt --dir "$check_dir"
cmp SHA256SUMS.txt "$check_dir/SHA256SUMS.txt"
(cd "$check_dir" && sha256sum --check SHA256SUMS.txt)
gh release edit "$tag" --draft=false
