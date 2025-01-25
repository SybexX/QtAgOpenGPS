
#!/bin/bash

# Get the name of the current branch
current_branch=$(git branch --show-current)

# Check if we're in a Git repository
if [ -z "$current_branch" ]; then
  echo "Error: Not in a Git repository or no branch is checked out."
  exit 1
fi

# Check if the -a flag is passed
include_remotes=false
if [[ $1 == "-a" ]]; then
  include_remotes=true
fi

# Function to compare branches
compare_branches() {
  local branch_name=$1
  local display_name=$2

  # Get ahead and behind counts
  ahead_behind=$(git rev-list --left-right --count $current_branch...$branch_name 2>/dev/null)
  ahead=$(echo $ahead_behind | awk '{print $1}')
  behind=$(echo $ahead_behind | awk '{print $2}')

  # Print the comparison if it's valid
  if [[ -n $ahead && -n $behind ]]; then
    printf "%-50s Behind: %-10s Ahead: %-10s\n" "$display_name" "$ahead" "$behind"
  fi
}

echo "Comparing all local branches to the current branch: $current_branch"
echo "--------------------------------------------------------------------------------"

# Compare against all local branches
for branch in $(git for-each-ref --format="%(refname:short)" refs/heads); do
  if [ "$branch" != "$current_branch" ]; then
    compare_branches "$branch" "Local: $branch"
  fi
done

# Check if remote branches should be included
if [ "$include_remotes" = true ]; then
  echo
  echo "Comparing all remote branches to the current branch: $current_branch"
  echo "--------------------------------------------------------------------------------"

  # Compare against all remote branches
  for remote_branch in $(git for-each-ref --format="%(refname:short)" refs/remotes); do
    compare_branches "$remote_branch" "Remote: $remote_branch"
  done
fi
