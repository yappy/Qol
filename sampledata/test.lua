--[[
void drawTexture(
	int dx, int dy, bool lrInv = false, bool udInv = false,
	int sx = 0, int sy = 0, int sw = -1, int sh = -1,
	int cx = 0, int cy = 0, float angle = 0.0f,
	float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);
]]--
trace.write("sinki", "sinki", "return", "sinki", "sinki", "gg");

local frame = 0;
local w, h;

function load(resource)
	trace.write(resource);
	resource:addFont(0, "testfont", "ＭＳ 明朝", "A", "Z", 16, 32);
	resource:addFont(0, "testj", "メイリオ", "あ", "ん", 128, 128);

	resource:addTexture(0, "unyo", "../sampledata/test_400_300.png");
	resource:addTexture(0, "ball", "../sampledata/circle.png");
end

function start()
	w, h = graph:getTextureSize(0, "unyo");
	trace.write("w=" .. w, "h=" .. h);

	sound:playBgm("../sampledata/Epoq-Lepidoptera.ogg");
end

function update()
	frame = frame + 1;
end

function draw()
	local t = frame * 3 % 512;
	graph:drawTexture(0, "unyo", 500, 350, false, false, 0, 0, -1, -1, w / 2, h / 2,
		frame / 3.14 / 10);
	graph:drawTexture(0, "ball", t, t, false, false, 0, 0, -1, -1, 0, 0,
		0.0, t / 512.0, t / 512.0, t / 512.0);

	graph:drawString(0, "testj", "ほ", 100, 200, 0x0000ff);
	graph:drawString(0, "testj", "ほわいと", 100, 600, 0x000000, -32);
end

return a, b, c, d;
