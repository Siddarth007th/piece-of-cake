FROM node:24.14.0-bookworm-slim AS build
WORKDIR /app
COPY Web/package.json Web/package-lock.json ./
RUN npm ci --no-audit --no-fund
COPY Web/ ./
RUN npm run build && npm prune --omit=dev

FROM node:24.14.0-bookworm-slim
ENV NODE_ENV=production
WORKDIR /app
COPY --from=build --chown=node:node /app/node_modules ./node_modules
COPY --from=build --chown=node:node /app/dist ./dist
COPY --from=build --chown=node:node /app/package.json /app/server.mjs /app/server-utils.mjs ./
USER node
HEALTHCHECK --interval=20s --timeout=3s CMD node -e "fetch('http://127.0.0.1:8080/healthz').then(r=>process.exit(r.ok?0:1)).catch(()=>process.exit(1))"
CMD ["node", "server.mjs"]
