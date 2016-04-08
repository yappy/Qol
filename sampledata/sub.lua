
trace.write("alice", "takenoko", "takenoko", "alice");
-- error("test error");

-- inf loop test
-- while true do end

-- file test
local a, b, c = sys.readFile("takenoko.txt");
trace.write("---takenoko.txt (3 lines)---");
trace.write(a, b, c);
trace.write("---takenoko.txt END---");

local x = { sys.readFile("takenoko.txt") };
trace.write("---takenoko.txt (all)---");
for i = 1, #x do
	trace.write(x[i]);
end
trace.write("---takenoko.txt END---");
