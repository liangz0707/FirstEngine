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

        private IntPtr _engineHandle;
        private IntPtr _viewportHandle;
        private bool _initialized = false;
        private HwndSource? _hwndSource;

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
                _engineHandle = EditorAPI_CreateEngine(windowHandle, width, height);
                if (_engineHandle == IntPtr.Zero)
                    return false;

                if (!EditorAPI_InitializeEngine(_engineHandle))
                {
                    EditorAPI_DestroyEngine(_engineHandle);
                    _engineHandle = IntPtr.Zero;
                    return false;
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
            if (!_initialized || _engineHandle == IntPtr.Zero)
                return false;

            try
            {
                _viewportHandle = EditorAPI_CreateViewport(_engineHandle, parentWindowHandle, x, y, width, height);
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

        public void BeginFrame()
        {
            if (_initialized && _engineHandle != IntPtr.Zero)
            {
                EditorAPI_BeginFrame(_engineHandle);
            }
        }

        public void EndFrame()
        {
            if (_initialized && _engineHandle != IntPtr.Zero)
            {
                EditorAPI_EndFrame(_engineHandle);
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
            if (_initialized && _engineHandle != IntPtr.Zero)
            {
                EditorAPI_LoadScene(_engineHandle, scenePath);
            }
        }

        public void UnloadScene()
        {
            if (_initialized && _engineHandle != IntPtr.Zero)
            {
                EditorAPI_UnloadScene(_engineHandle);
            }
        }

        public void SetRenderConfig(int width, int height, float renderScale = 1.0f)
        {
            if (_initialized && _engineHandle != IntPtr.Zero)
            {
                EditorAPI_SetRenderConfig(_engineHandle, width, height, renderScale);
            }
        }

        public void Dispose()
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                EditorAPI_DestroyViewport(_viewportHandle);
                _viewportHandle = IntPtr.Zero;
            }

            if (_initialized && _engineHandle != IntPtr.Zero)
            {
                EditorAPI_ShutdownEngine(_engineHandle);
                EditorAPI_DestroyEngine(_engineHandle);
                _engineHandle = IntPtr.Zero;
                _initialized = false;
            }

            if (_hwndSource != null)
            {
                _hwndSource.Dispose();
                _hwndSource = null;
            }
        }
    }
}
