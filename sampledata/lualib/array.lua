--[[
	配列ユーティリティ
	table 標準ライブラリも参照
]]

-- Export to global table
array = {};

--[[
	1次元配列を文字列として連結する。
	@param a 配列
	@return 文字列
]]
function array.tostring(a)
	local str = "[";
	local len = #a;
	for i, v in ipairs(a) do
		str = str .. v;
		if i ~= len then
			str = str .. ", ";
		end
	end
	str = str .. "]";
	return str;
end

--[[
	配列の末尾に要素を追加する。
	@param a 配列
	@param val 追加する値
]]
function array.push(a, val)
	a[#a + 1] = val;
end

--[[
	配列の末尾から要素を取得して削除する。
	@param a 配列
	@return 削除した値
]]
function array.pop(a)
	local last = #a;
	local val = a[last];
	a[last] = nil;
	return val;
end

--[[
	配列の a[1] .. a[n] を指定の値で埋める。
	@param a 配列
	@param n 要素数
	@param val 埋める値
]]
function array.fill(a, n, val)
	for i = 1, n do
		a[i] = val;
	end
end

--[[
	配列の一部をコピーして新たな配列を作成する。
	デフォルトでは全体をコピーする。
	@param a 配列
	@param from 開始インデックス(含む) (default=1)
	@param to 終了インデックス(含む) (default=#a)
	@return 新たな配列
]]
function array.copy(a, from, to)
	from = from or 1;
	to = to or #a;
	local t = {};
	for i = 1, to - from + 1 do
		t[i] = a[from + i - 1];
	end
	return t;
end

--[[
	配列を連結して新たな配列を作る。
	@param ... 配列のリスト
	@return 全てを連結してできた新たな配列
]]
function array.concat(...)
	local arrays = {...};
	local t = {};
	local ind = 1;
	for i, a in ipairs(arrays) do
		for i, v in ipairs(a) do
			t[ind] = v;
			ind = ind + 1;
		end
	end
	return t;
end

--[[
	配列を反転させる。
	@param 書き換える配列
]]
function array.reverse(a)
	local len = #a;
	for i = 1, len // 2 do
		a[i], a[len + 1 - i] = a[len + 1 - i], a[i];
	end
end

--[[
	配列をソートする。
	table.sort() と同じなのでそちらを参照。
]]
function array.sort(a, comp)
	return table.sort(a, comp);
end

--[[
	配列の先頭から指定の要素を探す。
	@param a 配列
	@param val 検索する値
	@return 見つかったインデックス、見つからなければ nil
]]
function array.find(a, val)
	for i = 1, #a do
		if a[i] == val then
			return i;
		end
	end
	return nil;
end

--[[
	配列の末尾から指定の要素を探す。
	@param a 配列
	@param val 検索する値
	@return 見つかったインデックス、見つからなければ nil
]]
function array.rfind(a, val)
	for i = #a, 1, -1 do
		if a[i] == val then
			return i;
		end
	end
	return nil;
end

--[[
	関数を呼びだしながら配列を作成する。
	@param n 要素数
	@param func 要素ごとにインデックスを引数として func(i) が呼び出される
		返り値がそのインデックスの値となる
	@return 作成された配列
]]
function array.generate(n, func)
	local t = {};
	for i = 1, n do
		t[i] = func(i);
	end
	return t;
end

--[[
	配列の先頭から関数が真を返す要素を探し、そのインデックスを返す。
	@param a 配列
	@param func func(a[i]) が真を返すものを探す
	@return 見つかったインデックス、見つからなければ nil
]]
function array.findif(a, func)
	for i = 1, #a do
		if func(a[i]) then
			return i;
		end
	end
	return nil;
end

--[[
	条件を満たす要素のみからなる新たな配列を作成する。
	@param a 元の配列
	@param func それぞれの要素を引数に func(a[i]) が呼び出される
		この関数が真を返したもののみ残る
	@return フィルタリングされてできた新たな配列
]]
function array.filter(a, func)
	local t = {};
	for i = 1, #a do
		if func(a[i]) then
			array.push(t, a[i]);
		end
	end
	return t;
end

--[[
	配列の要素を引数に左から関数を適用していって最終的な値を得る。
	@param a 配列
	@param func 計算に用いる2項関数
		それぞれの要素を引数に func(current_result, a[i]) が呼び出される
	@param init 初期値
	@return フィルタリングされてできた新たな配列
]]
function array.reduce(a, func, init)
	local ret = init;
	for i = 1, #a do
		ret = func(ret, a[i]);
	end
	return ret;
end

--[[
	配列のそれぞれの要素に関数を適用して得られる要素からなる新たな配列を作成する。
	@param a 配列
	@param func 値の変換関数
		それぞれの要素を引数に func(a[i]) が呼び出される
	@return 変換後の配列
]]
function array.map(a, func)
	local t = {};
	for i = 1, #a do
		t[i] = func(a[i]);
	end
	return t;
end



local function test()
	print(array.tostring({ 5, 4, 3, 2, 1 }));
	print(array.tostring({}));
	print(array.tostring({ 1 }));

	print(array.tostring(array.generate(
		5,
		function(i)
			return i * 2;
		end)));

	local target = { 1, 2, 3, 4, 5, 4, 3, 2, 1 };
	print(array.tostring(target));
	print(array.find(target, 3));
	print(array.rfind(target, 3));
	print(array.tostring(array.concat({ 1, 2, 3 }, { 1, 2 }, { 1 })));
	local fib = { 1, 1, 2, 3, 5, 8, 13 };
	array.reverse(fib);
	print(array.tostring(fib));

	print(array.tostring(array.filter(target,
		function(x)
			return x > 3;
		end)));
	print(array.findif(target,
		function(x)
			return x > 3;
		end));
	print(array.tostring(array.map(target,
		function(x)
			return x * 2;
		end)));
	print(array.reduce({ 1, 2, 3 },
		function(x, y)
			return x + y;
		end, 0));
	print(array.tostring(array.map({ 1, 2, 3 },
		function(x)
			return "takenoko" .. x;
		end)));
end
test();
