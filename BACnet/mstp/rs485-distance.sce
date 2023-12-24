// Scilab 3.0
// data from the graph
// The data is linear, but the graph is logarithmic
t = [0.1 0.2 0.4 0.5 0.8 1.0];
y = [4000 2000 1000 800 500 400];
xyd = [t;y];
yi=exp(interpln(log(xyd),log(t)));
err = (y-yi)'
tt = [0.1 0.1152 0.15625 0.2 0.2304 0.3125 0.4 0.4608 0.5 0.8 1.0];
yi=exp(interpln(log(xyd),log(tt)));
xyi = [tt ; yi]'
ttt = [0.1:0.01:1.0];
yi=exp(interpln(log(xyd),log(ttt)));
xbasc(); plot2d(ttt,yi); xgrid()
xtitle('RS-485 Distance vs Rate','Rate (Mbps)','Distance (feet)');


