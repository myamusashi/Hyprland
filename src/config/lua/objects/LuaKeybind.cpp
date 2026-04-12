#include "LuaKeybind.hpp"

#include <string_view>

using namespace Config::Lua;

static constexpr const char* MT = "HL.Keybind";

static int                   keybindSetEnabled(lua_State* L) {
    auto* ref = static_cast<WP<SKeybind>*>(luaL_checkudata(L, 1, MT));
    luaL_checktype(L, 2, LUA_TBOOLEAN);

    const auto keybind = ref->lock();
    if (!keybind)
        return 0;

    keybind->enabled = lua_toboolean(L, 2);
    return 0;
}

static int keybindIsEnabled(lua_State* L) {
    auto*      ref = static_cast<WP<SKeybind>*>(luaL_checkudata(L, 1, MT));

    const auto keybind = ref->lock();
    if (!keybind) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushboolean(L, keybind->enabled);
    return 1;
}

static int keybindIndex(lua_State* L) {
    luaL_checkudata(L, 1, MT);
    const std::string_view key = luaL_checkstring(L, 2);

    if (key == "set_enabled")
        lua_pushcfunction(L, keybindSetEnabled);
    else if (key == "is_enabled")
        lua_pushcfunction(L, keybindIsEnabled);
    else
        lua_pushnil(L);

    return 1;
}

void Objects::CLuaKeybind::setup(lua_State* L) {
    registerMetatable(L, MT, keybindIndex, gcRef<WP<SKeybind>>);
}

void Objects::CLuaKeybind::push(lua_State* L, const SP<SKeybind>& keybind) {
    new (lua_newuserdata(L, sizeof(WP<SKeybind>))) WP<SKeybind>(keybind);
    luaL_getmetatable(L, MT);
    lua_setmetatable(L, -2);
}
