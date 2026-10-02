#!/bin/sh
set -eu

src="isomorphisms/p-adic"
dst_owner="isomorphismes"
name="p-adic"
dst="$dst_owner/$name"

command -v gh >/dev/null 2>&1 || {
  echo "error: gh is required" >&2
  exit 127
}

gh auth status >/dev/null

gh repo view "$src" --json nameWithOwner >/dev/null || {
  echo "error: source repo $src is not accessible" >&2
  exit 1
}

if gh repo view "$dst" --json nameWithOwner >/dev/null 2>&1; then
  echo "error: target repo $dst already exists" >&2
  exit 2
fi

echo "Transferring $src -> $dst"
gh api   --method POST   "/repos/$src/transfer"   -f "new_owner=$dst_owner"   --jq '.full_name // .name'

echo "Transfer requested."
echo "New canonical URL: https://github.com/$dst"
