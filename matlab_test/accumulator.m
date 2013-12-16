t = linspace(0,10,1000);
avg_value = 300;
data = ones(size(t)) * avg_value;
jitter = floor(rand(size(t)) * 50);
data = data + jitter;
random_indices = ceil(rand(1,50)*length(data));
for i=1:length(random_indices)
    rand_ind = random_indices(i);
    data(rand_ind) = ceil(rand() * 65472);
end
plot(t,data);
alpha = 0.2;
accumulator = ones(1,length(t))*300;

for i = 2:length(t)
    %accumulator(i) = (alpha * x(i)) + (1.0 - alpha) * accumulator(i-1);
    if abs(data(i) - accumulator(i-1)) > 1000
        accumulator(i) = accumulator(i-1);
    else
        accumulator(i) = (1-alpha) * accumulator(i-1) + alpha * data(i);
    end
    
end

hold on;
plot(t,accumulator,'r');
ylim([290 360])