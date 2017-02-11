library(spatstat)

mydata <- read.csv(file="matingPairs.csv",head=FALSE,sep=",")
mypattern <- ppp(mydata[,1], mydata[,2],c(min(mydata[,1]),max(mydata[,1])),c(min(mydata[,2]),max(mydata[,2])))

png('ripleyKplot5.png')
plot(envelope(mypattern,Kest(correction="best")))
dev.off()
K = Kest(mypattern,correction = "best")

png('plotArea.png')
plot(mypattern)
dev.off()
