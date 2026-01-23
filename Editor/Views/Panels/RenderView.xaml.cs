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
                // Force layout update to get accurate size
                RenderSurface.UpdateLayout();
                RenderSurface.Measure(new System.Windows.Size(double.PositiveInfinity, double.PositiveInfinity));
                RenderSurface.Arrange(new Rect(RenderSurface.DesiredSize));
                
                var width = (int)RenderSurface.ActualWidth;
                var height = (int)RenderSurface.ActualHeight;
                
                // If size is still invalid, use DesiredSize or default
                if (width <= 0 || height <= 0)
                {
                    width = (int)(RenderSurface.DesiredSize.Width > 0 ? RenderSurface.DesiredSize.Width : 800);
                    height = (int)(RenderSurface.DesiredSize.Height > 0 ? RenderSurface.DesiredSize.Height : 600);
                }
                
                // Ensure minimum size
                if (width <= 0) width = 800;
                if (height <= 0) height = 600;

                // Initialize engine
                if (_renderEngine.Initialize(windowHandle, width, height))
                {
                    // Set render engine reference in SceneManagementService
                    var sceneService = ServiceLocator.Get<ISceneManagementService>();
                    if (sceneService is SceneManagementService sceneManagementService)
                    {
                        SceneManagementService.SetRenderEngine(_renderEngine);
                    }

                    // Wait for window to be fully loaded and laid out before creating viewport
                    // This ensures the HwndHost parent is ready and has correct size
                    Dispatcher.BeginInvoke(new Action(() =>
                    {
                        // Force another layout pass to ensure accurate size
                        RenderSurface.UpdateLayout();
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
            // Wait a bit to ensure layout is complete
            if (!_isInitialized && _renderEngine != null)
            {
                Dispatcher.BeginInvoke(new Action(() =>
                {
                    // Force layout update to get accurate size
                    RenderSurface.UpdateLayout();
                    CreateViewportHost();
                }), System.Windows.Threading.DispatcherPriority.Loaded);
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
                // Force layout update to get accurate size
                RenderSurface.UpdateLayout();
                RenderSurface.Measure(new System.Windows.Size(double.PositiveInfinity, double.PositiveInfinity));
                RenderSurface.Arrange(new Rect(RenderSurface.DesiredSize));
                
                var width = (int)RenderSurface.ActualWidth;
                var height = (int)RenderSurface.ActualHeight;
                
                // If size is still invalid, use DesiredSize or default
                if (width <= 0 || height <= 0)
                {
                    width = (int)(RenderSurface.DesiredSize.Width > 0 ? RenderSurface.DesiredSize.Width : 800);
                    height = (int)(RenderSurface.DesiredSize.Height > 0 ? RenderSurface.DesiredSize.Height : 600);
                }
                
                // Ensure minimum size
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

                        // Start render loop automatically after viewport is created
                        StartRenderLoop();

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
                // Get actual size from the render surface
                RenderSurface.UpdateLayout();
                var newWidth = (int)RenderSurface.ActualWidth;
                var newHeight = (int)RenderSurface.ActualHeight;
                
                // Fallback to event size if ActualWidth/Height is invalid
                if (newWidth <= 0) newWidth = (int)e.NewSize.Width;
                if (newHeight <= 0) newHeight = (int)e.NewSize.Height;
                
                // Ensure minimum size
                if (newWidth <= 0) newWidth = 800;
                if (newHeight <= 0) newHeight = 600;
                
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
                // Force layout update to get accurate size
                RenderSurface.UpdateLayout();
                RenderSurface.Measure(new System.Windows.Size(double.PositiveInfinity, double.PositiveInfinity));
                RenderSurface.Arrange(new Rect(RenderSurface.DesiredSize));
                
                var width = (int)RenderSurface.ActualWidth;
                var height = (int)RenderSurface.ActualHeight;
                
                // If size is still invalid, use DesiredSize
                if (width <= 0 || height <= 0)
                {
                    width = (int)(RenderSurface.DesiredSize.Width > 0 ? RenderSurface.DesiredSize.Width : 800);
                    height = (int)(RenderSurface.DesiredSize.Height > 0 ? RenderSurface.DesiredSize.Height : 600);
                }
                
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
                // 使用新的分离方法，对应 RenderApp 的流程
                _renderEngine.PrepareFrameGraph();
                _renderEngine.CreateResources();
                _renderEngine.Render();
                _renderEngine.SubmitFrame();
            }
        }

        private void OnRenderSurfaceDragOver(object sender, System.Windows.DragEventArgs e)
        {
            // Allow dropping scene or scene level resources
            if (e.Data.GetDataPresent("ResourceBrowserItem"))
            {
                if (e.Data.GetData("ResourceBrowserItem") is ResourceBrowserItem item)
                {
                    if (item.Type == "Scene" || item.Type == "SceneLevel")
                    {
                        e.Effects = System.Windows.DragDropEffects.Link;
                        e.Handled = true;
                        return;
                    }
                }
            }
            e.Effects = System.Windows.DragDropEffects.None;
        }

        private async void OnRenderSurfaceDrop(object sender, System.Windows.DragEventArgs e)
        {
            if (!e.Data.GetDataPresent("ResourceBrowserItem"))
                return;

            if (e.Data.GetData("ResourceBrowserItem") is ResourceBrowserItem item)
            {
                var sceneService = ServiceLocator.Get<ISceneManagementService>();
                
                if (item.Type == "Scene")
                {
                    // Load scene
                    string scenePath = GetFullPathFromVirtualPath(item.Path);
                    if (!string.IsNullOrEmpty(scenePath) && System.IO.File.Exists(scenePath))
                    {
                        bool success = await sceneService.LoadSceneAsync(scenePath);
                        if (!success)
                        {
                            System.Windows.MessageBox.Show("Failed to load scene", "Error", 
                                System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                        }
                    }
                }
                else if (item.Type == "SceneLevel")
                {
                    // Load scene level
                    if (string.IsNullOrEmpty(sceneService.GetCurrentScenePath()))
                    {
                        System.Windows.MessageBox.Show("No scene loaded. Please load a scene first.", "Error", 
                            System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Warning);
                        return;
                    }

                    string levelPath = GetFullPathFromVirtualPath(item.Path);
                    if (!string.IsNullOrEmpty(levelPath) && System.IO.File.Exists(levelPath))
                    {
                        bool success = await sceneService.LoadSceneLevelAsync(levelPath);
                        if (!success)
                        {
                            System.Windows.MessageBox.Show("Failed to load scene level", "Error", 
                                System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                        }
                    }
                }
            }
            e.Handled = true;
        }

        private string GetFullPathFromVirtualPath(string virtualPath)
        {
            var project = ServiceLocator.Get<IProjectManager>().CurrentProject;
            if (project == null) return string.Empty;

            string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
            if (string.IsNullOrEmpty(packagePath)) return string.Empty;

            // Convert virtual path to physical path
            // Virtual path format: "Scenes/scene.json" or similar
            // Path might already be a full path or relative path
            if (System.IO.Path.IsPathRooted(virtualPath))
            {
                // Normalize the path (convert to absolute, resolve .. and .)
                try
                {
                    return System.IO.Path.GetFullPath(virtualPath);
                }
                catch
                {
                    return virtualPath;
                }
            }
            
            // Combine package path with virtual path
            string normalizedVirtualPath = virtualPath.Replace('/', System.IO.Path.DirectorySeparatorChar);
            string fullPath = System.IO.Path.Combine(packagePath, normalizedVirtualPath);
            
            // Normalize the final path
            try
            {
                return System.IO.Path.GetFullPath(fullPath);
            }
            catch
            {
                return fullPath;
            }
        }
    }
}
