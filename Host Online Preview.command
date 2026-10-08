#!/bin/zsh
cd "${0:A:h}"
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
python3 Scripts/host_preview.py
result=$?
if (( result != 0 )); then
  read '?Hosting stopped. Press Return to close.'
fi
exit $result
