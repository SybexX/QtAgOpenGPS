#!/bin/bash

# Get the name of the current branch
current_branch=$(git branch --show-current)

# Check if we're in a Git repository
if [ -z "$current_branch" ]; then
  echo "Error: Not in a Git repository or no branch is checked out."
  exit 1
fi

# Loop through all local branches and compare them to the current branch
echo "Comparing all local branches to the current branch: $current_branch"
echo "--------------------------------------------------------------------"

for branch in $(git for-each-ref --format="%(refname:short)" refs/heads); do
  if [ "$branch" != "$current_branch" ]; then
    # Get ahead and behind counts
    ahead_behind=$(git rev-list --left-right --count $current_branch...$branch 2>/dev/null)
    ahead=$(echo $ahead_behind | awk '{print $1}')
    behind=$(echo $ahead_behind | awk '{print $2}')

    # Print the comparison
    printf "%-30s Behind: %-10s Ahead: %-10s\n" "$branch" "$ahead" "$behind"
  fi
done
