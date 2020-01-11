FROM arm64v8/node:alpine

ENV TZ America/New_York

WORKDIR /usr/src/app

COPY package.json .
COPY package-lock.json .
COPY index.js .
COPY node_modules node_modules

CMD ["npm", "start"]