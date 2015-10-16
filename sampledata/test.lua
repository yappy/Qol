a, b = "alice", "takenoko";
c = { a = 2, b = 3 };
d = function()
	local t = 3.14;
end

trace.write("sinki", "sinki", "return", "sinki", "sinki", "gg");
local w, h = graph:getTextureSize("notpow2");
trace.write("w=" .. w, "h=" .. h);

function draw()
	graph:drawTexture("testtex", 100, 100);
end

return a, b, c, d;
