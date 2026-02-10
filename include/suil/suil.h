// Copyright 2011-2017 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/// @file suil.h Public API for Suil

#ifndef SUIL_SUIL_H
#define SUIL_SUIL_H

#include <lv2/core/lv2.h>

#include <stdbool.h>
#include <stdint.h>

// SUIL_LIB_IMPORT and SUIL_LIB_EXPORT mark the entry points of shared libraries
#ifdef _WIN32
#  define SUIL_LIB_IMPORT __declspec(dllimport)
#  define SUIL_LIB_EXPORT __declspec(dllexport)
#else
#  define SUIL_LIB_IMPORT __attribute__((visibility("default")))
#  define SUIL_LIB_EXPORT __attribute__((visibility("default")))
#endif

// SUIL_API exposes symbols in the public API
#ifndef SUIL_API
#  ifdef SUIL_STATIC
#    define SUIL_API
#  elif defined(SUIL_INTERNAL)
#    define SUIL_API SUIL_LIB_EXPORT
#  else
#    define SUIL_API SUIL_LIB_IMPORT
#  endif
#endif

#if defined(__clang__) && __clang_major__ >= 7
#  define SUIL_NONNULL _Nonnull
#  define SUIL_NULLABLE _Nullable
#  define SUIL_ALLOCATED _Null_unspecified
#  define SUIL_UNSPECIFIED _Null_unspecified
#else
#  define SUIL_NONNULL
#  define SUIL_NULLABLE
#  define SUIL_ALLOCATED
#  define SUIL_UNSPECIFIED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
   @defgroup suil Suil C API
   @{
*/

/**
   @defgroup suil_callbacks Callbacks
   @{
*/

/**
   UI controller.

   This is an opaque pointer passed by the user which is passed to the various
   UI control functions (e.g. SuilPortWriteFunc).  It is typically used to pass
   a pointer to some controller object the host uses to communicate with
   plugins.
*/
typedef void* SUIL_UNSPECIFIED SuilController;

/// Function to write/send a value to a port
typedef void (*SuilPortWriteFunc)( //
  SuilController               controller,
  uint32_t                     port_index,
  uint32_t                     buffer_size,
  uint32_t                     protocol,
  void const* SUIL_UNSPECIFIED buffer);

/// Function to return the index for a port by symbol
typedef uint32_t (*SuilPortIndexFunc)( //
  SuilController           controller,
  const char* SUIL_NONNULL port_symbol);

/// Function to subscribe to notifications for a port
typedef uint32_t (*SuilPortSubscribeFunc)( //
  SuilController                                           controller,
  uint32_t                                                 port_index,
  uint32_t                                                 protocol,
  const LV2_Feature* SUIL_NULLABLE const* SUIL_UNSPECIFIED features);

/// Function to unsubscribe from notifications for a port
typedef uint32_t (*SuilPortUnsubscribeFunc)( //
  SuilController                                           controller,
  uint32_t                                                 port_index,
  uint32_t                                                 protocol,
  const LV2_Feature* SUIL_NULLABLE const* SUIL_UNSPECIFIED features);

/// Function called when a control is grabbed or released
typedef void (*SuilTouchFunc)( //
  SuilController controller,
  uint32_t       port_index,
  bool           grabbed);

/**
   @}
   @defgroup suil_library Library
   @{
*/

/// Initialization argument
typedef enum { SUIL_ARG_NONE } SuilArg;

/**
   Initialize suil.

   This function should be called as early as possible, before any other GUI
   toolkit functions.  The variable argument list is a sequence of SuilArg keys
   and corresponding value pairs for passing any necessary platform-specific
   information.  It must be terminated with SUIL_ARG_NONE.
*/
SUIL_API void
suil_init(int* SUIL_NONNULL                                argc,
          char* SUIL_NONNULL * SUIL_NONNULL * SUIL_NONNULL argv,
          SuilArg                                          key,
          ...);

/**
   Check if suil can wrap a UI type.

   @param host_type_uri The URI of the desired widget type of the host,
   corresponding to the `type_uri` parameter of suil_instance_new().

   @param ui_type_uri The URI of the UI widget type.

   @return 0 if wrapping is unsupported, otherwise the quality of the wrapping
   where 1 is the highest quality (direct native embedding with no wrapping)
   and increasing values are of a progressively lower quality and/or stability.
*/
SUIL_API unsigned
suil_ui_supported(const char* SUIL_NONNULL host_type_uri,
                  const char* SUIL_NONNULL ui_type_uri);

/**
   @}
   @defgroup suil_host Host
   @{
*/

/**
   UI host descriptor.

   This contains the various functions that a plugin UI may use to communicate
   with the plugin.  It is passed to suil_instance_new() to provide these
   functions to the UI.
*/
typedef struct SuilHostImpl SuilHost;

/**
   Create a new UI host descriptor.

   @param write_func Function to send a value to a plugin port.
   @param index_func Function to get the index for a port by symbol.
   @param subscribe_func Function to subscribe to port updates.
   @param unsubscribe_func Function to unsubscribe from port updates.
*/
SUIL_API SuilHost* SUIL_ALLOCATED
suil_host_new(SuilPortWriteFunc SUIL_NONNULL        write_func,
              SuilPortIndexFunc SUIL_NULLABLE       index_func,
              SuilPortSubscribeFunc SUIL_NULLABLE   subscribe_func,
              SuilPortUnsubscribeFunc SUIL_NULLABLE unsubscribe_func);

/**
   Set a touch function for a host descriptor.

   Note this function will only be called if the UI supports it.
*/
SUIL_API void
suil_host_set_touch_func(SuilHost* SUIL_NONNULL      host,
                         SuilTouchFunc SUIL_NULLABLE touch_func);

/**
   Free `host`.
*/
SUIL_API void
suil_host_free(SuilHost* SUIL_NULLABLE host);

/**
   @}
*/

/**
   @defgroup suil_instance Instance
   @{
*/

/// An instance of an LV2 plugin UI
typedef struct SuilInstanceImpl SuilInstance;

/// Opaque pointer to a UI handle
typedef void* SUIL_UNSPECIFIED SuilHandle;

/// Opaque pointer to a UI widget
typedef void* SUIL_UNSPECIFIED SuilWidget;

/**
   Instantiate a UI for an LV2 plugin.

   This function may load a suil module to adapt the UI to the desired toolkit.
   Suil is configured at compile time to load modules from the appropriate
   place, but this can be changed at run-time via the environment variable
   SUIL_MODULE_DIR.  This makes it possible to bundle suil with an application.

   Note that some situations (Gtk in Qt, Windows in Gtk) require a parent
   container to be passed as a feature with URI LV2_UI__parent
   (http://lv2plug.in/ns/extensions/ui#ui) in order to work correctly.  The
   data must point to a single child container of the host widget set.

   @param host Host descriptor.
   @param controller Opaque host controller pointer.
   @param container_type_uri URI of the desired host container widget type.
   @param plugin_uri URI of the plugin to instantiate this UI for.
   @param ui_uri URI of the specifically desired UI.
   @param ui_type_uri URI of the actual UI widget type.
   @param ui_bundle_path Path of the UI bundle.
   @param ui_binary_path Path of the UI binary.
   @param features NULL-terminated array of supported features, or NULL.
   @return A new UI instance, or NULL if instantiation failed.
*/
SUIL_API SuilInstance* SUIL_ALLOCATED
suil_instance_new(SuilHost* SUIL_NONNULL    host,
                  SuilController            controller,
                  const char* SUIL_NULLABLE container_type_uri,
                  const char* SUIL_NONNULL  plugin_uri,
                  const char* SUIL_NONNULL  ui_uri,
                  const char* SUIL_NONNULL  ui_type_uri,
                  const char* SUIL_NONNULL  ui_bundle_path,
                  const char* SUIL_NONNULL  ui_binary_path,
                  const LV2_Feature* SUIL_NULLABLE const* SUIL_NULLABLE
                    features);

/**
   Free a plugin UI instance.

   The caller must ensure all references to the UI have been dropped before
   calling this function (e.g. it has been removed from its parent).
*/
SUIL_API void
suil_instance_free(SuilInstance* SUIL_NULLABLE instance);

/**
   Get the handle for a UI instance.

   Returns the handle to the UI instance.  The returned handle has opaque type
   to insulate the Suil API from LV2 extensions, but in pactice it is currently
   of type `LV2UI_Handle`.  This should not normally be needed.

   The returned handle is shared and must not be deleted.
*/
SUIL_API SuilHandle
suil_instance_get_handle(SuilInstance* SUIL_NONNULL instance);

/**
   Get the widget for a UI instance.

   Returns an opaque pointer to a widget, the type of which matches the
   `container_type_uri` parameter of suil_instance_new().  Note this may be a
   wrapper widget created by Suil, and not necessarily the widget directly
   implemented by the UI.
*/
SUIL_API SuilWidget
suil_instance_get_widget(SuilInstance* SUIL_NONNULL instance);

/**
   Notify the UI about a change in a plugin port.

   This function can be used to notify the UI about any port change, but in the
   simplest case is used to set the value of lv2:ControlPort ports.  For
   simplicity, this is a special case where `format` is 0, `buffer_size` is 4,
   and `buffer` should point to a single float.

   The `buffer` must be valid only for the duration of this call, the UI must
   not keep a reference to it.

   @param instance UI instance.
   @param port_index Index of the port which has changed.
   @param buffer_size Size of `buffer` in bytes.
   @param format Format of `buffer` (mapped URI, or 0 for float).
   @param buffer Change data, e.g. the new port value.
*/
SUIL_API void
suil_instance_port_event(SuilInstance* SUIL_NONNULL   instance,
                         uint32_t                     port_index,
                         uint32_t                     buffer_size,
                         uint32_t                     format,
                         const void* SUIL_UNSPECIFIED buffer);

/// Return a data structure defined by some LV2 extension URI
SUIL_API const void* SUIL_UNSPECIFIED
suil_instance_extension_data(SuilInstance* SUIL_NONNULL instance,
                             const char* SUIL_NONNULL   uri);

/**
   @}
   @}
*/

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* SUIL_SUIL_H */
