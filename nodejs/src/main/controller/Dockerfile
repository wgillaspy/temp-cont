# 4
FROM arm32v7/ubuntu:latest

ENV TZ America/New_York

WORKDIR /usr/src/app

RUN apt-get update
RUN apt-get install -y python make g++

COPY node-v10.16.2-linux-armv7l.tar.xz ./node-v10.16.2-linux-armv7l.tar.xz
RUN tar xvf node-v10.16.2-linux-armv7l.tar.xz
RUN rm -f node-v10.16.2-linux-armv7l.tar.xz

ENV PATH /usr/src/app/node-v10.16.2-linux-armv7l/bin/:$PATH

COPY package*.json ./
COPY index.js ./

RUN npm --unsafe-perm install

CMD ["npm", "start"]
