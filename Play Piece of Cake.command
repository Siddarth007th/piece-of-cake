#!/bin/zsh
cd "${0:A:h}"
python3 Scripts/play_browser.py
result=$?
if (( result != 0 )); then
  print '
The game did not start. The reason is shown above.'
  read '?Press Return to close this window.'
fi
exit $result
