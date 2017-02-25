library(spatstat)


fname <- file.choose()
mydata <- read.csv(file=fname,head=FALSE,sep=",")
mypattern <- ppp(mydata[,1], mydata[,2],c(min(mydata[,1]),max(mydata[,1])),c(min(mydata[,2]),max(mydata[,2])))

png('ripleyKplot.png')
plot(envelope(mypattern,Kest(correction="best")))
dev.off()

png('nestingArea.png')
plot(mypattern)
dev.off()

