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
*/
namespace export {

	/** @brief デバッグ出力関数。<b>trace</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * trace = {};
	 * @endcode
	 * デバッグ出力を本体側のロギングシステムに転送します。
	 *
	 * @sa @ref yappy::debug
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

	/** @brief 乱数関連関数。<b>rand</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * rand = {};
	 * @endcode
	 */
	struct rand {
		static int generateSeed(lua_State *L);
		static int setSeed(lua_State *L);
		static int nextInt(lua_State *L);
		static int nextDouble(lua_State *L);
		rand() = delete;
	};
	const luaL_Reg rand_RegList[] = {
		{ "generateSeed",	rand::generateSeed },
		{ "setSeed",		rand::setSeed		},
		{ "nextInt",		rand::nextInt		},
		{ "nextDouble",		rand::nextDouble	},
		{ nullptr, nullptr }
	};

	/** @brief 使用リソース登録関数。
	 * @details
	 * リソースはリソースセットID(整数)とリソースID(文字列)で識別されます。
	 * リソースの登録は @ref yappy::framework::Application::sealResource() で
	 * 許可されている間にしかできません(違反するとエラーが発生します)。
	 * リソースを登録した後、C++側で
	 * @ref yappy::framework::Application::loadResourceSet()
	 * を呼ぶとリソースが使用可能になります。
	 * @sa @ref yappy::framework::Application
	 * @sa @ref yappy::framework::ResourceManager
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

	/** @brief グラフィックス描画関連関数。<b>graph</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * graph = {};
	 * @endcode
	 * 描画するテクスチャリソースはリソースセットID(整数)とリソースID(文字列)で
	 * 指定します。
	 * @sa @ref yappy::lua::export::resource
	 * @sa @ref yappy::graphics::DGraphics
	 */
	struct graph {
		static int getParam(lua_State *L);
		static int getTextureSize(lua_State *L);
		static int drawTexture(lua_State *L);
		static int drawString(lua_State *L);
		graph() = delete;
	};
	const luaL_Reg graph_RegList[] = {
		{ "getParam",		graph::getParam			},
		{ "getTextureSize",	graph::getTextureSize	},
		{ "drawTexture",	graph::drawTexture		},
		{ "drawString",		graph::drawString		},
		{ nullptr, nullptr }
	};

	/** @brief 音声再生関連関数。<b>sound</b>グローバルテーブルに提供。
	 * @details
	 * @code
	 * sound = {};
	 * @endcode
	 * @sa @ref yappy::lua::export::resource
	 * @sa @ref yappy::sound::XAudio2
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

}
}
}
