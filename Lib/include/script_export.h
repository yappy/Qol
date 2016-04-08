#pragma once

#include <lua.h>
#include <lauxlib.h>

/** @file
 * @brief Luaへエクスポートする関数群。
 * @details
 * ドキュメントを読みやすくするためにクラスの中に各関数を入れています。
 * 基本的にC++クラス名をLuaグローバルテーブル変数名に対応させています。
 */

 // Each function is documented in script_export.cpp

namespace yappy {
namespace lua {
/** @brief C++ から Lua へ公開する関数。(Lua関数仕様)
* @sa lua::Lua
*/
namespace export {

	/** @brief デバッグ出力関数。<b>trace</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * trace = {};
	 * @endcode
	 * デバッグ出力を本体側のロギングシステムに転送します。
	 *
	 * @sa debug
	 */
	struct trace {
		static int write(lua_State *L);
		static int perf(lua_State *L);
		trace() = delete;
	};
	const luaL_Reg trace_RegList[] = {
		{ "write",	trace::write	},
		{ "perf",	trace::perf		},
		{ nullptr, nullptr			}
	};

	/** @brief システム関連関数。<b>sys</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * sys = {};
	 * @endcode
	 */
	struct sys {
		static int include(lua_State *L);
		static int readFile(lua_State *L);
		static int writeFile(lua_State *L);
		sys() = delete;
	};
	const luaL_Reg sys_RegList[] = {
		{ "include",	sys::include	},
		{ "readFile",	sys::readFile	},
		{ "writeFile",	sys::writeFile	},
		{ nullptr, nullptr }
	};

	/** @brief 使用リソース登録関数。
	 * @details
	 * 各関数は最初の引数に self オブジェクトが必要です。
	 * リソースはリソースセットID(整数)とリソースID(文字列)で識別されます。
	 * 何らかの初期化関数の引数としてテーブルが渡されます(今のところ)。
	 * その中でリソースを登録した後、C++側で
	 * framework::Application::loadResourceSet()
	 * を呼ぶとリソースが使用可能になります。
	 * @sa framework::Application
	 * @sa framework::ResourceManager
	 */
	struct resource {
		static int addTexture(lua_State *L);
		static int addFont(lua_State *L);
		static int addSe(lua_State *L);
		static int addBgm(lua_State *L);
	};
	const luaL_Reg resource_RegList[] = {
		{ "addTexture",	resource::addTexture	},
		{ "addFont",	resource::addFont		},
		{ "addSe",		resource::addSe			},
		{ "addBgm",		resource::addBgm		},
		{ nullptr, nullptr }
	};
	const char *const resource_RawFieldName = "_rawptr";

	/** @brief グラフィックス描画関連関数。<b>graph</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * graph = {};
	 * @endcode
	 * 各関数は最初の引数に self オブジェクトが必要です。
	 * 描画するテクスチャリソースはリソースセットID(整数)とリソースID(文字列)で
	 * 指定します。
	 * @sa lua::export::resource
	 * @sa graphics::DGraphics
	 */
	struct graph {
		static int getTextureSize(lua_State *L);
		static int drawTexture(lua_State *L);
		static int drawString(lua_State *L);
		graph() = delete;
	};
	const luaL_Reg graph_RegList[] = {
		{ "getTextureSize",	graph::getTextureSize	},
		{ "drawTexture",	graph::drawTexture		},
		{ "drawString",		graph::drawString		},
		{ nullptr, nullptr }
	};
	const char *const graph_RawFieldName = "_rawptr";

	/** @brief 音声再生関連関数。<b>sound</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * sound = {};
	 * @endcode
	 * 各関数は最初の引数に self オブジェクトが必要です。
	 * @sa lua::export::resource
	 * @sa sound::XAudio2
	 */
	struct sound {
		static int playSe(lua_State *L);
		static int playBgm(lua_State *L);
		static int stopBgm(lua_State *L);
		sound() = delete;
	};
	const luaL_Reg sound_RegList[] = {
		{ "playSe",		sound::playSe	},
		{ "playBgm",	sound::playBgm	},
		{ "stopBgm",	sound::stopBgm	},
		{ nullptr, nullptr }
	};
	const char *const sound_RawFieldName = "_rawptr";

}
}
}
