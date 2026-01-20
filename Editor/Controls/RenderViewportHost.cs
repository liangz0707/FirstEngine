using System;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using HandleRef = System.Runtime.InteropServices.HandleRef;

namespace FirstEngineEditor.Controls
{
    /// <summary>
    /// HwndHost wrapper for embedding native render viewport window
    /// </summary>
    public class RenderViewportHost : HwndHost
    {
        private IntPtr _viewportHandle;
        private const int WS_CHILD = 0x40000000;
        private const int WS_VISIBLE = 0x10000000;
        private const int WS_CLIPSIBLINGS = 0x04000000;
        private const int WS_CLIPCHILDREN = 0x02000000;
        private const int WS_EX_CLIENTEDGE = 0x00000200;

        public IntPtr ViewportHandle
        {
            get => _viewportHandle;
            set
            {
                if (_viewportHandle != value)
                {
                    _viewportHandle = value;
                    if (Handle != IntPtr.Zero)
                    {
                        // Reparent the viewport window to this HwndHost
                        ReparentViewport();
                    }
                }
            }
        }

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            if (_viewportHandle != IntPtr.Zero)
            {
                // Reparent the existing viewport window to this HwndHost
                var reparentedHandle = ReparentViewportWindow(_viewportHandle, hwndParent.Handle);
                return new HandleRef(this, reparentedHandle);
            }
            else
            {
                // Create a placeholder window (should not happen in normal flow)
                // This means viewport was not created yet
                System.Diagnostics.Debug.WriteLine("Warning: Viewport handle is zero, creating placeholder");
                return CreatePlaceholderWindow(hwndParent);
            }
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            // Don't destroy the viewport window here - it's managed by C++ code
            // The viewport window will be destroyed by EditorAPI_DestroyViewport
            // We just need to ensure it's properly cleaned up
            _viewportHandle = IntPtr.Zero;
        }

        private IntPtr ReparentViewportWindow(IntPtr viewportHandle, IntPtr newParent)
        {
            if (viewportHandle == IntPtr.Zero)
                return IntPtr.Zero;

            // Reparent the viewport window to the new parent (HwndHost)
            SetParent(viewportHandle, newParent);
            
            // Update window style to be a child window (if not already)
            long styleLong = GetWindowLong(viewportHandle, GWL_STYLE);
            int style = (int)styleLong;
            if ((style & (int)WS_CHILD) == 0)
            {
                style = (int)((uint)style & ~((uint)WS_POPUP | (uint)WS_CAPTION | (uint)WS_THICKFRAME | (uint)WS_MINIMIZEBOX | (uint)WS_MAXIMIZEBOX | (uint)WS_SYSMENU)) 
                        | (int)WS_CHILD | (int)WS_VISIBLE | (int)WS_CLIPSIBLINGS | (int)WS_CLIPCHILDREN;
                SetWindowLong(viewportHandle, GWL_STYLE, style);
            }

            // Update extended style
            long exStyleLong = GetWindowLong(viewportHandle, GWL_EXSTYLE);
            int exStyle = (int)exStyleLong;
            exStyle = (int)((uint)exStyle & ~(uint)WS_EX_DLGMODALFRAME) | (int)WS_EX_CLIENTEDGE;
            SetWindowLong(viewportHandle, GWL_EXSTYLE, exStyle);

            // Position and size the window to fill the HwndHost
            const uint SWP_NOZORDER = 0x0004;
            const uint SWP_NOACTIVATE = 0x0010;
            
            var width = (int)(ActualWidth > 0 ? ActualWidth : 100);
            var height = (int)(ActualHeight > 0 ? ActualHeight : 100);
            
            SetWindowPos(viewportHandle, IntPtr.Zero, 0, 0, 
                        width, height,
                        SWP_NOZORDER | SWP_NOACTIVATE);

            return viewportHandle;
        }

        private HandleRef CreatePlaceholderWindow(HandleRef hwndParent)
        {
            // Create a placeholder window (should not be used in production)
            var hwnd = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                "STATIC",
                "RenderViewport Placeholder",
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                0, 0, 100, 100,
                hwndParent.Handle,
                IntPtr.Zero,
                IntPtr.Zero,
                IntPtr.Zero
            );

            return new HandleRef(this, hwnd);
        }

        private void ReparentViewport()
        {
            if (_viewportHandle != IntPtr.Zero && Handle != IntPtr.Zero)
            {
                var hwndParent = new HandleRef(this, Handle);
                ReparentViewportWindow(_viewportHandle, hwndParent.Handle);
            }
        }

        protected override void OnRenderSizeChanged(SizeChangedInfo sizeInfo)
        {
            base.OnRenderSizeChanged(sizeInfo);

            if (_viewportHandle != IntPtr.Zero && Handle != IntPtr.Zero)
            {
                const uint SWP_NOZORDER = 0x0004;
                const uint SWP_NOACTIVATE = 0x0010;

                var width = (int)sizeInfo.NewSize.Width;
                var height = (int)sizeInfo.NewSize.Height;
                
                if (width > 0 && height > 0)
                {
                    SetWindowPos(_viewportHandle, IntPtr.Zero, 0, 0,
                                width, height,
                                SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        }

        #region Win32 P/Invoke

        private const int GWL_STYLE = -16;
        private const int GWL_EXSTYLE = -20;
        private const uint WS_POPUP = 0x80000000;
        private const uint WS_CAPTION = 0x00C00000;
        private const uint WS_THICKFRAME = 0x00040000;
        private const uint WS_MINIMIZEBOX = 0x00020000;
        private const uint WS_MAXIMIZEBOX = 0x00010000;
        private const uint WS_SYSMENU = 0x00080000;
        private const uint WS_EX_DLGMODALFRAME = 0x00000001;
        private const uint WS_POPUPWINDOW = WS_POPUP | WS_BORDER | WS_SYSMENU;
        private const uint WS_BORDER = 0x00800000;

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

        [System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
        private static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

        [System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
        private static extern long GetWindowLong(IntPtr hWnd, int nIndex);

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        [System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern IntPtr CreateWindowEx(
            uint dwExStyle,
            string lpClassName,
            string lpWindowName,
            uint dwStyle,
            int x, int y,
            int nWidth, int nHeight,
            IntPtr hWndParent,
            IntPtr hMenu,
            IntPtr hInstance,
            IntPtr lpParam
        );

        #endregion
    }
}
