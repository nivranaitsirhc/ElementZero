#pragma once

#include <string>
#include <map>
#include <optional>
#include <type_traits>

#include <boost/format.hpp>

#include <Core/json.h>
#include <Item/Enchant.h>

#include <base.h>
#include <chakra_helper.h>
#include <playerdb.h>

#ifdef ScriptingSupport_EXPORTS
#  define SCRIPTAPI __declspec(dllexport)
#else
#  define SCRIPTAPI __declspec(dllimport)
#endif

class ScriptEngine;

template <> SCRIPTAPI ScriptEngine *LocateService<ScriptEngine>();

namespace Mod::Scripting {

SCRIPTAPI JsValueRef ToJs(Json::Value entry);
template <> SCRIPTAPI Json::Value FromJs(JsValueRef ref);

struct RegisterQueue {
  SCRIPTAPI static std::map<std::string, void (*)(JsObjectWarpper global)> &GetList();

  SCRIPTAPI RegisterQueue(char const *name, void (*t)(JsObjectWarpper global));
};

struct ModuleRegister {
  SCRIPTAPI static std::map<std::string, std::string (*)(JsObjectWarpper module)> &GetList();

  SCRIPTAPI ModuleRegister(char const *name, std::string (*t)(JsObjectWarpper global));
};

struct ScriptAuxData : Mod::AuxHolder {
  ValueHolder holder;
  ScriptAuxData() {
    JsObjectWarpper wrapper;
    holder.ref = *wrapper;
  }
};

struct PlayerBinding {
  Mod::PlayerEntry entry;

  inline PlayerBinding() {}

  inline PlayerBinding(Mod::PlayerEntry entry) : entry(entry) {}

  inline std::string GetXUID() const { return std::to_string(entry.xuid); }
  inline std::string GetUUID() const { return entry.uuid.asString(); }
  inline std::string GetNAME() const { return entry.name; }
  inline std::string GetADDRESS() const { return entry.netid.getRealAddress().ToString(); }
  inline JsValueRef GetAuxData() const {
    return *Mod::PlayerDatabase::GetInstance().GetAuxAuto<ScriptAuxData>(entry.player).holder;
  }

  inline Mod::OfflinePlayerEntry ToOffline() { return Mod::OfflinePlayerEntry{entry.name, entry.xuid, entry.uuid}; }
  inline bool alive() const { return Mod::PlayerDatabase::GetInstance().Find(entry.xuid).has_value(); }

  inline std::string ToString() {
    return (boost::format("Player { xuid: %d, uuid: %s, name: %s, ip: %s }") % entry.xuid % GetUUID() % GetNAME() %
            GetADDRESS())
        .str();
  }

  SCRIPTAPI static JsValueRef InitProto();

  inline static JsObjectWarpper Create(Mod::PlayerEntry entry) {
    return JsObjectWarpper::FromExternalObject(new PlayerBinding(entry), InitProto());
  }
};

inline JsValueRef ToJs(Mod::PlayerEntry entry) { return *PlayerBinding::Create(entry); }
template <> struct HasToJs<Mod::PlayerEntry> : std::true_type {};

template <> inline Mod::PlayerEntry FromJs(JsValueRef ref) {
  JsValueRef tmp;
  ThrowError(JsGetPrototype(ref, &tmp));
  if (tmp == PlayerBinding::InitProto()) {
    PlayerBinding *bd;
    ThrowError(JsGetExternalData(ref, (void **) &bd));
    if (bd) return bd->entry;
  }
  throw std::runtime_error{"null data"};
}

struct OfflinePlayerBinding {
  Mod::OfflinePlayerEntry entry;

  inline OfflinePlayerBinding() {}
  inline OfflinePlayerBinding(Mod::OfflinePlayerEntry entry) : entry(entry) {}

  inline std::string GetXUID() const { return std::to_string(entry.xuid); }
  inline std::string GetUUID() const { return entry.uuid.asString(); }
  inline std::string GetNAME() const { return entry.name; }

  inline std::optional<Mod::PlayerEntry> ToOnline() { return Mod::PlayerDatabase::GetInstance().Find(entry.xuid); }

  inline std::string ToString() {
    return (boost::format("OfflinePlayer { xuid: %d, uuid: %s, name: %s }") % entry.xuid % GetUUID() % GetNAME()).str();
  }

  SCRIPTAPI static JsValueRef InitProto();

  inline static JsObjectWarpper Create(Mod::OfflinePlayerEntry entry) {
    return JsObjectWarpper::FromExternalObject(new OfflinePlayerBinding(entry), InitProto());
  }
};

inline JsValueRef ToJs(Mod::OfflinePlayerEntry entry) { return *OfflinePlayerBinding::Create(entry); }
template <> struct HasToJs<Mod::OfflinePlayerEntry> : std::true_type {};

template <> inline Mod::OfflinePlayerEntry FromJs(JsValueRef ref) {
  JsValueRef tmp;
  ThrowError(JsGetPrototype(ref, &tmp));
  if (tmp == OfflinePlayerBinding::InitProto()) {
    OfflinePlayerBinding *bd;
    ThrowError(JsGetExternalData(ref, (void **) &bd));
    if (bd) return bd->entry;
  }
  throw std::runtime_error{"null data"};
}

struct ScriptNBT {
  std::unique_ptr<Tag> storage;

  inline ScriptNBT(std::unique_ptr<Tag> const &tag) : storage(tag->copy()) {}

  inline int type() const { return storage->getId(); }

  inline std::string toString() { return storage->toString(); }

  inline bool equals(std::unique_ptr<Tag> const &rhs) { return storage->equals(*rhs); }

  inline JsValueRef clone() { return *ScriptNBT::Create(storage); }

  SCRIPTAPI static JsValueRef InitProto();

  inline static JsObjectWarpper Create(std::unique_ptr<Tag> const &tag) {
    return JsObjectWarpper::FromExternalObject(new ScriptNBT{tag}, InitProto());
  }
};
inline JsValueRef ToJs(std::unique_ptr<Tag> const &tag) { return *ScriptNBT::Create(tag); }
template <> struct HasToJs<std::unique_ptr<Tag>> : std::true_type {};
template <> inline std::unique_ptr<Tag> FromJs(JsValueRef ref) {
  JsValueRef tmp;
  ThrowError(JsGetPrototype(ref, &tmp));
  if (tmp == ScriptNBT::InitProto()) {
    ScriptNBT *bd;
    ThrowError(JsGetExternalData(ref, (void **) &bd));
    if (bd) return bd->storage->copy();
  }
  throw std::runtime_error{"null data"};
}

struct ScriptEnchantmentInstance {
  EnchantmentInstance instance;

  inline int GetType() const { return (int) instance.type; }

  ScriptEnchantmentInstance(EnchantmentInstance const &instance) : instance(instance) {}

  static JsValueRef InitProto();

  inline static JsObjectWarpper Create(EnchantmentInstance const &instance) {
    return JsObjectWarpper::FromExternalObject(new ScriptEnchantmentInstance{instance}, InitProto());
  }
};

inline JsValueRef ToJs(EnchantmentInstance const &stack) { return *ScriptEnchantmentInstance::Create(stack); }
template <> struct HasToJs<EnchantmentInstance> : std::true_type {};

struct ScriptItemStack {
  ItemStack stack;
  inline ScriptItemStack(ItemStack const &stack) : stack(stack) {}

  inline JsValueRef GetEnchants() const {
    auto vec = stack.getEnchantsFromUserData().getAllEnchants();
    return ToJsArray(vec);
  }

  inline bool Equals(ItemStack const &rhs) { return !(stack != rhs); }

  static JsValueRef InitProto();

  inline static JsObjectWarpper Create(ItemStack const &stack) {
    return JsObjectWarpper::FromExternalObject(new ScriptItemStack{stack}, InitProto());
  }
};
inline JsValueRef ToJs(ItemStack const &stack) {
  if (stack.isNull()) return GetNullValue();
  return *ScriptItemStack::Create(stack);
}
template <> struct HasToJs<ItemStack> : std::true_type {};
template <> inline ItemStack FromJs(JsValueRef ref) {
  if (GetJsType(ref) == JsNull) return ItemStack{};
  JsValueRef tmp;
  ThrowError(JsGetPrototype(ref, &tmp));
  if (tmp == ScriptItemStack::InitProto()) {
    ScriptItemStack *bd;
    ThrowError(JsGetExternalData(ref, (void **) &bd));
    if (bd) return bd->stack;
  }
  throw std::runtime_error{"null data"};
}

} // namespace Mod::Scripting