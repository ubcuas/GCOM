FROM node:20-slim
# RUN mkdir -p /frontend
WORKDIR /frontend
COPY package*.json .
RUN npm install


COPY . .
RUN npm run build

EXPOSE 1324
CMD ["npm", "run", "dev"]

