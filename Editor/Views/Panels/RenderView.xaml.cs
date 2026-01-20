using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Media;
using FirstEngineEditor.ViewModels.Panels;
using FirstEngineEditor.Services;
using FirstEngineEditor.Controls;

namespace FirstEngineEditor.Views.Panels
{
    public partial class RenderView : System.Windows.Controls.UserControl
    {
        private RenderEngineService? _renderEngine;
        private RenderViewportHost? _viewportHost;
        private bool _isInitialized = false;

        public RenderView()
        {
            InitializeComponent();
            Loaded += OnLoaded;
            Unloaded += OnUnloaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            // Initialize render engine when control is loaded
            InitializeRenderEngine();
        }

        private void OnUnloaded(object sender, RoutedEventArgs e)
        {
            // Cleanup render engine when control is unloaded
            CleanupRenderEngine();
        }

        private void InitializeRenderEngine()
        {
            if (_isInitialized)
                return;

            try
            {
                // Get window handle
                var window = Window.GetWindow(this);
                if (window == null)
                    return;

                var windowHandle = new WindowInteropHelper(window).Handle;
                if (windowHandle == IntPtr.Zero)
                    return;

                // Create render engine service
                _renderEngine = new RenderEngineService();
                
                // Get actual size of render surface
                RenderSurface.UpdateLayout();
                var width = (int)RenderSurface.ActualWidth;
                var height = (int)RenderSurface.ActualHeight;
                
                if (width <= 0) width = 800;
                if (height <= 0) height = 600;

                // Initialize engine
                if (_renderEngine.Initialize(windowHandle, width, height))
                {
                    // Wait for window to be fully loaded before creating viewport
                    // This ensures the HwndHost parent is ready
                    Dispatcher.BeginInvoke(new Action(() =>
                    {
                        CreateViewportHost();
                    }), System.Windows.Threading.DispatcherPriority.Loaded);
                }
                else
                {
                    System.Diagnostics.Debug.WriteLine("Failed to initialize render engine");
                    if (DataContext is RenderViewModel viewModel)
                    {
                        viewModel.RenderStatus = "Engine Initialization Failed";
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error initializing render engine: {ex.Message}");
                if (DataContext is RenderViewModel viewModel)
                {
                    viewModel.RenderStatus = $"Error: {ex.Message}";
                }
            }
        }

        private void CleanupRenderEngine()
        {
            // Remove viewport host from container
            if (_viewportHost != null)
            {
                RenderHostContainer.Children.Remove(_viewportHost);
                _viewportHost = null;
            }
            
            if (_renderEngine != null)
            {
                _renderEngine.Dispose();
                _renderEngine = null;
                _isInitialized = false;
            }
        }

        private void OnRenderHostLoaded(object sender, RoutedEventArgs e)
        {
            // Render host container is loaded
            if (!_isInitialized && _renderEngine != null)
            {
                CreateViewportHost();
            }
        }
        
        private void CreateViewportHost()
        {
            if (_isInitialized || _renderEngine == null)
                return;

            try
            {
                // Get window handle
                var window = Window.GetWindow(this);
                if (window == null)
                    return;

                var windowHandle = new WindowInteropHelper(window).Handle;
                if (windowHandle == IntPtr.Zero)
                    return;

                // Get actual size of render surface
                RenderSurface.UpdateLayout();
                var width = (int)RenderSurface.ActualWidth;
                var height = (int)RenderSurface.ActualHeight;
                
                if (width <= 0) width = 800;
                if (height <= 0) height = 600;

                // Create viewport (this will create a child window in C++)
                if (_renderEngine.CreateViewport(windowHandle, 0, 0, width, height))
                {
                    // Get the viewport window handle
                    var viewportHandle = _renderEngine.GetViewportWindowHandle();
                    
                    if (viewportHandle != IntPtr.Zero)
                    {
                        // Create HwndHost to embed the viewport window
                        _viewportHost = new RenderViewportHost
                        {
                            ViewportHandle = viewportHandle,
                            Width = width,
                            Height = height
                        };
                        
                        // Add to container
                        RenderHostContainer.Children.Clear();
                        RenderHostContainer.Children.Add(_viewportHost);
                        
                        _isInitialized = true;

                        if (DataContext is RenderViewModel viewModel)
                        {
                            viewModel.RenderStatus = "Engine Initialized - Viewport Embedded";
                        }
                    }
                    else
                    {
                        System.Diagnostics.Debug.WriteLine("Failed to get viewport window handle");
                        if (DataContext is RenderViewModel viewModel)
                        {
                            viewModel.RenderStatus = "Failed to get viewport handle";
                        }
                    }
                }
                else
                {
                    System.Diagnostics.Debug.WriteLine("Failed to create viewport");
                    if (DataContext is RenderViewModel viewModel)
                    {
                        viewModel.RenderStatus = "Failed to create viewport";
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error creating viewport host: {ex.Message}");
                if (DataContext is RenderViewModel viewModel)
                {
                    viewModel.RenderStatus = $"Error: {ex.Message}";
                }
            }
        }

        private void OnRenderHostSizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (_renderEngine != null && _isInitialized)
            {
                var newWidth = (int)e.NewSize.Width;
                var newHeight = (int)e.NewSize.Height;
                
                if (newWidth > 0 && newHeight > 0)
                {
                    // Resize viewport (child window will be resized automatically)
                    _renderEngine.ResizeViewport(0, 0, newWidth, newHeight);
                    _renderEngine.SetRenderConfig(newWidth, newHeight, 1.0f);
                    
                    // Update viewport host size
                    if (_viewportHost != null)
                    {
                        _viewportHost.Width = newWidth;
                        _viewportHost.Height = newHeight;
                    }
                }
            }
        }
        
        private void UpdateViewportSize()
        {
            if (_renderEngine != null && _isInitialized && _viewportHost != null)
            {
                RenderSurface.UpdateLayout();
                var width = (int)RenderSurface.ActualWidth;
                var height = (int)RenderSurface.ActualHeight;
                
                if (width > 0 && height > 0)
                {
                    _renderEngine.ResizeViewport(0, 0, width, height);
                    _renderEngine.SetRenderConfig(width, height, 1.0f);
                    _viewportHost.Width = width;
                    _viewportHost.Height = height;
                }
            }
        }

        private void OnStartRender(object sender, RoutedEventArgs e)
        {
            if (!_isInitialized)
            {
                InitializeRenderEngine();
            }

            if (DataContext is RenderViewModel viewModel)
            {
                viewModel.StartRendering();
            }

            // Start render loop if not already running
            if (_renderEngine != null && _isInitialized)
            {
                StartRenderLoop();
            }
        }

        private void OnPauseRender(object sender, RoutedEventArgs e)
        {
            if (DataContext is RenderViewModel viewModel)
            {
                viewModel.PauseRendering();
            }
        }

        private void OnStopRender(object sender, RoutedEventArgs e)
        {
            if (DataContext is RenderViewModel viewModel)
            {
                viewModel.StopRendering();
            }
        }

        private void StartRenderLoop()
        {
            // Use CompositionTarget.Rendering for render loop
            CompositionTarget.Rendering += OnCompositionTargetRendering;
        }

        private void StopRenderLoop()
        {
            CompositionTarget.Rendering -= OnCompositionTargetRendering;
        }

        private void OnCompositionTargetRendering(object? sender, EventArgs e)
        {
            if (_renderEngine != null && _isInitialized)
            {
                _renderEngine.BeginFrame();
                _renderEngine.RenderViewport();
                _renderEngine.EndFrame();
            }
        }
    }
}
