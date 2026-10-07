#!/bin/zsh
set -eu
cd "${0:A:h}"
python3 Scripts/ue.py editor
