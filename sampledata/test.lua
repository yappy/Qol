--[[
void drawTexture(
	string id, int dx, int dy, bool lrInv = false, bool udInv = false,
	int sx = 0, int sy = 0, int sw = -1, int sh = -1,
	int cx = 0, int cy = 0, float angle = 0.0f,
	float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);
]]--
trace.write("sinki", "sinki", "return", "sinki", "sinki", "gg");

local frame = 0;
local w, h;

function init()
	graph:loadFont("testfont", "ＭＳ 明朝", "A", "Z", 16, 32);
	graph:loadFont("testj", "メイリオ", "あ", "ん", 128, 128);

	graph:loadTexture("unyo", "../sampledata/test_400_300.png");
	graph:loadTexture("ball", "../sampledata/circle.png");
	w, h = graph:getTextureSize("unyo");
	trace.write("w=" .. w, "h=" .. h);
end

function update()
	frame = frame + 1;
end

function draw()
	local t = frame * 3 % 512;
	graph:drawTexture("unyo", t, t, false, false, 0, 0, -1, -1, w / 2, h / 2,
		t / 3.14 / 20
		, t / 512.0, t / 512.0, t / 512.0);

	graph:drawString("testj", "ほ", 100, 200, 0x0000ff);
	graph:drawString("testj", "ほわいと", 100, 600, 0x000000, -32);
end

return a, b, c, d;
