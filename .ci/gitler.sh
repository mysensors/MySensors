#!/bin/bash
# Yes, this script can be improved immensly...
cd ..

result=0

echo "No subjects are of invalid size<br>" > too_long_subjects.txt
echo "No subjects with leading lower case characters<br>" > leading_lowercases.txt
echo "No subjects with trailing periods<br>" > trailing_periods.txt
echo "No body lines are too wide<br>" > too_long_body_lines.txt

too_long_subjects=`awk 'length > 50' subjects.txt`
if [ -n "$too_long_subjects" ]; then
  echo "<b>Commit subjects that are too wide (&gt;50 characters):</b>" > too_long_subjects.txt
  echo "$too_long_subjects" >> too_long_subjects.txt
  sed -i -e 's/$/<br>/' too_long_subjects.txt
  result=1
fi
leading_lowercases=`awk '/^[[:lower:][:punct:]]/' subjects.txt`
if [ -n "$leading_lowercases" ]; then
  echo "<b>Commit subjects with leading lowercase characters:</b>" > leading_lowercases.txt
  echo "$leading_lowercases" >> leading_lowercases.txt
  sed -i -e 's/$/<br>/' leading_lowercases.txt
  result=1
fi
trailing_periods=`awk '/(\.)$/' subjects.txt`
if [ -n "$trailing_periods" ]; then
  echo "<b>Commit subjects with trailing periods:</b>" > trailing_periods.txt
  echo "$trailing_periods" >> trailing_periods.txt
  sed -i -e 's/$/<br>/' trailing_periods.txt
  result=1
fi

too_long_body_lines=`awk 'length > 72' bodies.txt`
if [ -n "$too_long_body_lines" ]; then
  echo "<b>Body lines that are too wide (&gt;72 characters):</b>" > too_long_body_lines.txt
  echo "$too_long_body_lines" >> too_long_body_lines.txt
  sed -i -e 's/$/<br>/' too_long_body_lines.txt
  result=1
fi

printf "%s" "<html>" > gitler.html
awk 'FNR==1{print "<br>"}1' too_long_subjects.txt leading_lowercases.txt trailing_periods.txt too_long_body_lines.txt >> gitler.html
echo "<br>" >> gitler.html
if [ $result -ne 0 ]; then
	echo "You should follow <a href="http://chris.beams.io/posts/git-commit">this guide</a> when writing your commit messages.<br>" >> gitler.html
	echo "<br>" >> gitler.html
	echo "To change the commit message for a single-commit pull request:<br>" >> gitler.html
	echo "git checkout &lt;your_branch&gt;<br>" >> gitler.html
	echo "git commit --amend<br>" >> gitler.html
	echo "git push origin HEAD:&lt;your_branch_on_github&gt; -f<br>" >> gitler.html
	echo "<br>" >> gitler.html
	echo "To change the commit messages for a multiple-commit pull request:<br>" >> gitler.html
	echo "git checkout &lt;your_branch&gt;<br>" >> gitler.html
	echo "git rebase -i &lt;sha_of_parent_to_the_earliest_commit_you_want_to_change&gt;<br>" >> gitler.html
	echo "Replace \"pick\" with \"r\" or \"reword\" on the commits you need to change message for<br>" >> gitler.html
	echo "git push origin HEAD:&lt;your_branch_on_github&gt; -f<br>" >> gitler.html
	echo "<br>" >> gitler.html
fi

# Evaluate coding style
astyle --options=.mystools/astyle/config/style.cfg -nq --recursive "*.h" "*.c" "*.cpp"
git diff > restyling.patch
if [ -s restyling.patch ]; then
	echo "<b>This commit is not meeting the coding standards, a mail with a patch has been sent to you that if applied to your PR, will make it meet the coding standards.</b><br>" >> gitler.html
	echo "You apply the patch using:<br>" >> gitler.html
	echo "git apply restyling.patch<br>" >> gitler.html
	echo "<br>" >> gitler.html
	result=1
else
	echo "This commit is meeting the coding standards, congratulations!<br>" >> gitler.html
	echo "<br>" >> gitler.html
	rm restyling.patch
fi

# Evaluate if there exists booleans in the code tree (not counting this file)
if git grep -q boolean -- `git ls-files | grep -v gitler.sh`
then
	echo "<b>This repository currently contain the boolean keyword. This should be avoided.</b><br>" >> gitler.html
	echo "<br>" >> gitler.html
	result=1
else
	echo "This repository does not contain any boolean keywords. This is good.<br>" >> gitler.html
	echo "<br>" >> gitler.html
fi

if [ $result -ne 0 ]; then
	echo "<b>If you disagree to this, please discuss it in the GitHub pull request thread.</b><br>" >> gitler.html
	echo "<br>" >> gitler.html
fi
echo "Yours sincerely, Gitler, on behalf of Jenkins<br>" >> gitler.html
printf "%s" "</html>" >> gitler.html
exit $result
