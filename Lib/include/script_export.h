#pragma once

/** @file
 * @brief Lua�փG�N�X�|�[�g����֐��Q�B
 * @details
 * �h�L�������g��ǂ݂₷�����邽�߂ɃN���X�̒��Ɋe�֐������Ă��܂��B
 * ��{�I��C++�N���X����Lua�O���[�o���e�[�u���ϐ����ɑΉ������Ă��܂��B
 */

 // Each function is documented in script_export.cpp

namespace yappy {
namespace lua {
/** @brief C++ ���� Lua �֌��J����֐��B(Lua�֐��d�l)
* @sa lua::Lua
*/
namespace export {

	/** @brief �f�o�b�O�o�͊֐��B<b>trace</b>�O���[�o���e�[�u���ɒ񋟁B
	* @details
	* @code
	* trace = {};
	* @endcode
	* �f�o�b�O�o�͂�{�̑��̃��M���O�V�X�e���ɓ]�����܂��B
	*
	* @sa debug
	*/
	struct trace {
		static int write(lua_State *L);
		trace() = delete;
	};
	const luaL_Reg trace_RegList[] = {
		{ "write",	trace::write	},
		{ nullptr, nullptr			}
	};

	/** @brief �g�p���\�[�X�o�^�֐��B
	* @details
	* �e�֐��͍ŏ��̈����� self �I�u�W�F�N�g���K�v�ł��B
	* ���\�[�X�̓��\�[�X�Z�b�gID(����)�ƃ��\�[�XID(������)�Ŏ��ʂ���܂��B
	* ���炩�̏������֐��̈����Ƃ��ăe�[�u�����n����܂�(���̂Ƃ���)�B
	* ���̒��Ń��\�[�X��o�^������AC++����
	* framework::Application::loadResourceSet()
	* ���ĂԂƃ��\�[�X���g�p�\�ɂȂ�܂��B
	* @sa framework::Application
	* @sa framework::ResourceManager
	*/
	struct resource {
		static int addTexture(lua_State *L);
		static int addFont(lua_State *L);
		static int addSe(lua_State *L);
	};
	const luaL_Reg resource_RegList[] = {
		{ "addTexture",	resource::addTexture	},
		{ "addFont",	resource::addFont		},
		{ "addSe",		resource::addSe			},
		{ nullptr, nullptr }
	};
	const char *const resource_RawFieldName = "_rawptr";

	/** @brief �O���t�B�b�N�X�`��֘A�֐��B<b>graph</b>�O���[�o���e�[�u���ɒ񋟁B
	* @details
	* @code
	* graph = {};
	* @endcode
	* �e�֐��͍ŏ��̈����� self �I�u�W�F�N�g���K�v�ł��B
	* �`�悷��e�N�X�`�����\�[�X�̓��\�[�X�Z�b�gID(����)�ƃ��\�[�XID(������)��
	* �w�肵�܂��B
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

	/** @brief �����Đ��֘A�֐��B<b>sound</b>�O���[�o���e�[�u���ɒ񋟁B
	* @details
	* @code
	* sound = {};
	* @endcode
	* �e�֐��͍ŏ��̈����� self �I�u�W�F�N�g���K�v�ł��B
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
