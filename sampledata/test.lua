trace.write("sinki", "sinki", "return", "sinki", "sinki", "gg");

local frame = 0;
local w, h;

graph:loadTexture("notpow2", "../sampledata/test_400_300.png");
graph:loadTexture("testtex", "../sampledata/circle.png");
w, h = graph:getTextureSize("notpow2");
trace.write("w=" .. w, "h=" .. h);


function update()
	frame = frame + 1;
end

function draw()
	graph:drawTexture("testtex", frame * 5 % 512, frame * 5 % 512);
end

return a, b, c, d;
