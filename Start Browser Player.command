#!/bin/zsh
set -eu
cd "${0:A:h}/Web"
if [[ ! -d node_modules ]]; then npm ci; fi
npm run build
print 'Browser player: http://127.0.0.1:8080'
print 'This starts the player and signalling. Start Unreal separately to supply the game stream.'
exec node server.mjs
