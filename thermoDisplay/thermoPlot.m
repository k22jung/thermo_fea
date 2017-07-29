clear all
close all
clc

file = fullfile ('output.txt');
T = readtable(file,'Delimiter',' ','ReadVariableNames',false);
conLines = 100;
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
    
    if mod(t*dt,displaytime) == 0
        %tic;
        for y = 1:y_max
            row = ((y-1)*x_max+1):(y*x_max);
            grid(y,:) = T{t,row};
        end


        delete (h_old);
        [c,h]=contourf(grid,conLines);
        caxis([0,100]);
        title(['t = ', num2str(t*dt), 's', char(10), num2str(round(grid(round(x_max/2),1))), '°C']);
        x_lab = xlabel([num2str((round(grid(round(x_max/2),y_max)))), '°C']);
        x_lab.FontWeight = 'bold';
        y_lab = ylabel([num2str((round(grid(round(x_max,round(y_max/2)))))), '°C']);
        y_lab.FontWeight = 'bold';
        set(get(gca,'YLabel'),'Rotation',0)
       
        
        delete(findall(gcf,'Tag','ann'));
        a = annotation('textbox',...
        [0.92225 0.523076923076923 0.0365000000000001 0.0307692307692308],...
        'String',[num2str(round(grid(1,round(y_max/2)))), '°C'],...
        'FitBoxToText','off',...
        'EdgeColor','none','FontWeight','bold','Tag','ann');
        a.FontSize = 12;
        set(h,'EdgeColor','none');
        h_old = h;
        drawnow;

        %pause(dt);
        %display(toc);
    end
end