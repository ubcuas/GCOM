FROM node:20-slim
WORKDIR /frontend
COPY package.json package-lock.json ./
RUN npm ci


COPY . .
RUN npm run build

EXPOSE 1324
CMD ["npm", "run", "dev"]
