commit IDs that you've mentioned for restoring removed files:

be22674f8c1ac84e1cff89947ff4a6753070f21b
31e1d4f839118b59398ca6f871929fc0e286e13c
751a23af0f5a9612b8e28af1400896a3026ee331
9dbc79dc96a4cf439adbead5563e46d1eb301391
a5958bf7f0da207e02065a88355b8afae0b5e256


These are the commits from which we're trying to restore deleted files in your repository.


```
# Reset to a clean state for our branch
git reset --hard HEAD

# Extract ONLY the filenames (without the extra metadata)
git show be22674f8c1ac84e1cff89947ff4a6753070f21b --name-status | grep ^D | cut -f2- > deleted_commit1.txt
git show 31e1d4f839118b59398ca6f871929fc0e286e13c --name-status | grep ^D | cut -f2- > deleted_commit2.txt
git show 751a23af0f5a9612b8e28af1400896a3026ee331 --name-status | grep ^D | cut -f2- > deleted_commit3.txt
git show 9dbc79dc96a4cf439adbead5563e46d1eb301391 --name-status | grep ^D | cut -f2- > deleted_commit4.txt
git show a5958bf7f0da207e02065a88355b8afae0b5e256 --name-status | grep ^D | cut -f2- > deleted_commit5.txt

# Review the files to make sure they look correct
cat deleted_commit1.txt
cat deleted_commit2.txt
cat deleted_commit3.txt
cat deleted_commit4.txt
cat deleted_commit5.txt

# Now restore each file individually to avoid errors with empty files
while read file; do
  if [[ -n "$file" ]]; then
    git checkout be22674f8c1ac84e1cff89947ff4a6753070f21b~1 -- "$file"
  fi
done < deleted_commit1.txt

while read file; do
  if [[ -n "$file" ]]; then
    git checkout 31e1d4f839118b59398ca6f871929fc0e286e13c~1 -- "$file"
  fi
done < deleted_commit2.txt

while read file; do
  if [[ -n "$file" ]]; then
    git checkout 751a23af0f5a9612b8e28af1400896a3026ee331~1 -- "$file"
  fi
done < deleted_commit3.txt

while read file; do
  if [[ -n "$file" ]]; then
    git checkout 9dbc79dc96a4cf439adbead5563e46d1eb301391~1 -- "$file"
  fi
done < deleted_commit4.txt

while read file; do
  if [[ -n "$file" ]]; then
    git checkout a5958bf7f0da207e02065a88355b8afae0b5e256~1 -- "$file"
  fi
done < deleted_commit5.txt

# Add and commit all restored files
git add .
git commit -m "Restore files removed in commits be22674, 31e1d4f, 751a23a, 9dbc79d, and a5958bf"
```