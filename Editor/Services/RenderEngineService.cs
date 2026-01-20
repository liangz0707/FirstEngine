using System;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;

namespace FirstEngineEditor.Services
{
    /// <summary>
    /// P/Invoke wrapper for FirstEngine Editor API
    /// </summary>
    public class RenderEngineService : IDisposable
    {
        #region P/Invoke Declarations

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr EditorAPI_CreateEngine(IntPtr windowHandle, int width, int height);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_DestroyEngine(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool EditorAPI_InitializeEngine(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_ShutdownEngine(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr EditorAPI_CreateViewport(IntPtr engine, IntPtr parentWindowHandle, int x, int y, int width, int height);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_DestroyViewport(IntPtr viewport);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_ResizeViewport(IntPtr viewport, int x, int y, int width, int height);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_SetViewportActive(IntPtr viewport, [MarshalAs(UnmanagedType.I1)] bool active);

        // 新的分离方法（对应 RenderApp 的方法）
        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_PrepareFrameGraph(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_CreateResources(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_Render(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_SubmitFrame(IntPtr viewport);

        // 已废弃：为了向后兼容保留
        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_BeginFrame(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_EndFrame(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_RenderViewport(IntPtr viewport);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool EditorAPI_IsViewportReady(IntPtr viewport);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr EditorAPI_GetViewportWindowHandle(IntPtr viewport);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void EditorAPI_LoadScene(IntPtr engine, string scenePath);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_UnloadScene(IntPtr engine);

        [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void EditorAPI_SetRenderConfig(IntPtr engine, int width, int height, float renderScale);

        #endregion

        private IntPtr _viewportHandle;
        private bool _initialized = false;
        private HwndSource? _hwndSource;
        private static IntPtr? _globalEngineHandle = null;  // 全局引擎句柄（单例）

        public bool IsInitialized => _initialized;
        
        public IntPtr GetViewportWindowHandle()
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                return EditorAPI_GetViewportWindowHandle(_viewportHandle);
            }
            return IntPtr.Zero;
        }

        public bool Initialize(IntPtr windowHandle, int width, int height)
        {
            if (_initialized)
                return true;

            try
            {
                // 使用全局引擎句柄（单例模式）
                if (!_globalEngineHandle.HasValue || _globalEngineHandle.Value == IntPtr.Zero)
                {
                    _globalEngineHandle = EditorAPI_CreateEngine(windowHandle, width, height);
                    if (_globalEngineHandle.Value == IntPtr.Zero)
                        return false;

                    if (!EditorAPI_InitializeEngine(_globalEngineHandle.Value))
                    {
                        EditorAPI_DestroyEngine(_globalEngineHandle.Value);
                        _globalEngineHandle = null;
                        return false;
                    }
                }

                _initialized = true;
                return true;
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Failed to initialize render engine: {ex.Message}");
                return false;
            }
        }

        public bool CreateViewport(IntPtr parentWindowHandle, int x, int y, int width, int height)
        {
            if (!_initialized || !_globalEngineHandle.HasValue || _globalEngineHandle.Value == IntPtr.Zero)
                return false;

            try
            {
                _viewportHandle = EditorAPI_CreateViewport(_globalEngineHandle.Value, parentWindowHandle, x, y, width, height);
                return _viewportHandle != IntPtr.Zero && EditorAPI_IsViewportReady(_viewportHandle);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Failed to create viewport: {ex.Message}");
                return false;
            }
        }

        public void ResizeViewport(int x, int y, int width, int height)
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                EditorAPI_ResizeViewport(_viewportHandle, x, y, width, height);
            }
        }

        public void SetViewportActive(bool active)
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                EditorAPI_SetViewportActive(_viewportHandle, active);
            }
        }

        // 新的分离方法（对应 RenderApp 的方法）
        public void PrepareFrameGraph()
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_PrepareFrameGraph(_globalEngineHandle.Value);
            }
        }

        public void CreateResources()
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_CreateResources(_globalEngineHandle.Value);
            }
        }

        public void Render()
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_Render(_globalEngineHandle.Value);
            }
        }

        public void SubmitFrame()
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                EditorAPI_SubmitFrame(_viewportHandle);
            }
        }

        // 已废弃：为了向后兼容保留
        public void BeginFrame()
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_BeginFrame(_globalEngineHandle.Value);
            }
        }

        public void EndFrame()
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_EndFrame(_globalEngineHandle.Value);
            }
        }

        public void RenderViewport()
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                EditorAPI_RenderViewport(_viewportHandle);
            }
        }

        public void LoadScene(string scenePath)
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_LoadScene(_globalEngineHandle.Value, scenePath);
            }
        }

        public void UnloadScene()
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_UnloadScene(_globalEngineHandle.Value);
            }
        }

        public void SetRenderConfig(int width, int height, float renderScale = 1.0f)
        {
            if (_initialized && _globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_SetRenderConfig(_globalEngineHandle.Value, width, height, renderScale);
            }
        }

        public void Dispose()
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                EditorAPI_DestroyViewport(_viewportHandle);
                _viewportHandle = IntPtr.Zero;
            }

            // 注意：不在这里销毁全局引擎，因为可能有多个 RenderEngineService 实例
            // 全局引擎应该在应用程序退出时统一销毁
            _initialized = false;

            if (_hwndSource != null)
            {
                _hwndSource.Dispose();
                _hwndSource = null;
            }
        }

        // 静态方法：销毁全局引擎（应该在应用程序退出时调用）
        public static void ShutdownGlobalEngine()
        {
            if (_globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero)
            {
                EditorAPI_ShutdownEngine(_globalEngineHandle.Value);
                EditorAPI_DestroyEngine(_globalEngineHandle.Value);
                _globalEngineHandle = null;
            }
        }
    }
}
