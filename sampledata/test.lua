--[[
void drawTexture(resourceSetId, resourceId,
	int dx, int dy, bool lrInv = false, bool udInv = false,
	int sx = 0, int sy = 0, int sw = -1, int sh = -1,
	int cx = 0, int cy = 0, float angle = 0.0f,
	float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);

void drawString(resourceSetId, resourceId,
	str *str, int dx, int dy,
	uint32_t color = 0x000000, int ajustX = 0,
	float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f,
	/* int *nextx = nullptr, int *nexty = nullptr */);
]]--

trace.write("sinki", "sinki", "return", "sinki", "sinki", "gg");

local frame = 0;
local w, h;
local unyopos = {
	x = 500,
	y = 350
};

function load(resource)
	resource:addTexture(1, "unyo", "../sampledata/test_400_300.png");
	resource:addTexture(1, "ball", "../sampledata/circle.png");
end

function start()
	w, h = graph:getTextureSize(1, "unyo");
	trace.write("w=" .. w, "h=" .. h);

	sound:playBgm("../sampledata/Epoq-Lepidoptera.ogg");
end

function update(keyinput)
	frame = frame + 1;

	local amount = 4;
	if keyinput["UP"] then
		unyopos.y = unyopos.y - amount;
	end
	if keyinput["DOWN"] then
		unyopos.y = unyopos.y + amount;
	end
	if keyinput["LEFT"] then
		unyopos.x = unyopos.x - amount;
	end
	if keyinput["RIGHT"] then
		unyopos.x = unyopos.x + amount;
	end

	for k, v in pairs(keyinput) do
		if v then
			trace.write("Key input in lua: " .. k);
		end
	end
end

function draw()
	local t = frame * 3 % 512;
	graph:drawTexture(1, "unyo", unyopos.x, unyopos.y, false, false, 0, 0, -1, -1, w / 2, h / 2,
		frame / 3.14 / 10);
	graph:drawTexture(1, "ball", t, t, false, false, 0, 0, -1, -1, 0, 0,
		0.0, t / 512.0, t / 512.0, t / 512.0);

	graph:drawString(0, "j", "ほ", 100, 200, 0x0000ff);
	graph:drawString(0, "j", "ほわいと", 100, 500, 0x000000, -32);
	graph:drawString(0, "j", "やじるしでうごくよ", 500, 640, 0x000000, -80, 0.5, 0.5);
	graph:drawString(0, "e", "SPACE key: goto C++ impl scene", 0, 0);
end

return a, b, c, d;
