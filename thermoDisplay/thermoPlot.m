clear all
close all
clc

file = fullfile ('output.txt');
T = readtable(file,'Delimiter',' ','ReadVariableNames',false);
x_max = 50;
y_max = 50;
dt = 0.01; 
totalTime = 10;
grid = zeros([y_max x_max]);

iterations = totalTime/dt;

displaytime = 0.1;

figure('units','normalized','outerposition',[0 0 1 1])
[c,h]=contourf(grid,75);
caxis([0,100])
h_old = h;

for t = 1:iterations
    
    if t == iterations
        %tic;
        for y = 1:y_max
            row = ((y-1)*x_max+1):(y*x_max);
            grid(y,:) = T{t,row};
        end


        delete (h_old);
        [c,h]=contourf(grid,2000);
        caxis([0,75])
        set(h,'EdgeColor','none');
        h_old = h;
        drawnow;

        %pause(dt);
        %display(toc);
    end
end