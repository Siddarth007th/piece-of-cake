#!/bin/zsh
cd "${0:A:h}"
python3 Scripts/ue.py doctor
print '\nPress Return to close.'
read
