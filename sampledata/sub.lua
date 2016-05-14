print("print test");
trace.write("alice", "takenoko", "takenoko", "alice");
-- error("test error");

-- inf loop test
-- while true do end

-- file test
local a, b, c = sys.readFile("takenoko.txt");
trace.write("---takenoko.txt (3 lines)---");
trace.write(a, b, c);
trace.write("---takenoko.txt END---");

local list = { "hokuto", "basuke", "fatal ko" };
sys.writeFile("takenoko.txt", table.unpack(list));

local x = { sys.readFile("takenoko.txt") };
trace.write("---takenoko.txt (all)---");
for i = 1, #x do
	trace.write(x[i]);
end
trace.write("---takenoko.txt END---");

-- random test
print("Random Test");
print("Generate seed");
for i = 1, 5 do
	print(rand.generateSeed());
end

-- fixed seed
local seed = rand.generateSeed();
for i = 1, 2 do
	rand.setSeed(seed);
	print("random int [1, 5]");
	for i = 1, 10 do
		print(rand.nextInt(1, 5));
	end
	print("random double [0.0, 1.0)");
	for i = 1, 5 do
		print(rand.nextDouble());
	end
end
