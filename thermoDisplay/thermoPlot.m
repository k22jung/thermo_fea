clear all
close all
clc

file = fullfile ('output.txt');
T = readtable(file,'Delimiter',' ','ReadVariableNames',false);
conLines = 500;
x_max = 100;
y_max = 100;
dt = 0.01; 
totalTime = 10;
grid = zeros([y_max x_max]);

iterations = totalTime/dt;

displaytime = 0.05;

figure('Position', [300, 20, 800, 650])
[c,h]=contourf(grid,conLines);
caxis([0,100])
h_old = h;

for t = 1:iterations
    
    if t==iterations%mod(t*dt,displaytime) == 0
        %tic;
        for y = 1:y_max
            row = ((y-1)*x_max+1):(y*x_max);
            grid(y,:) = T{t,row};
        end


        delete (h_old);
        [c,h]=contourf(grid,conLines);
        caxis([0,100])
        set(h,'EdgeColor','none');
        h_old = h;
        drawnow;

        %pause(dt);
        %display(toc);
    end
end