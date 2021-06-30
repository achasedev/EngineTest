///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: November 29th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include "Game/Framework/App.h"
#include "Game/Framework/Game.h"
#include "Game/Framework/GameCommon.h"
#include "Engine/Event/EventSystem.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/Window.h"
#include "Engine/IO/File.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Time/Clock.h"
#include "Engine/Utility/StringID.h"
#include <optional>
#include <set>

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------
App* g_app = nullptr;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
bool AppMessageHandler(unsigned int msg, size_t wParam, size_t lParam)
{
	UNUSED(wParam);
	UNUSED(lParam);

	switch (msg)
	{
	case WM_CLOSE: // App close requested via "X" button, "Close Window" on task bar, or "Close" from system menu, or Alt-F4
	{
		g_app->Quit();
		return true;
	}
	case WM_KEYDOWN:
	{
		unsigned char asKey = (unsigned char)wParam;
		if (asKey == VK_ESCAPE)
		{
			g_app->Quit();
			return true;
		}
		break;
	}
	}

	return false;
}


//-------------------------------------------------------------------------------------------------
void RunMessagePump()
{
	MSG queuedMessage;
	for (;;)
	{
		const BOOL wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call the "WindowsMessageHandlingProcedure" above
	}
}


///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS IMPLEMENTATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
App::App()
{
	
}


//-------------------------------------------------------------------------------------------------
App::~App()
{
	ShutdownVulkan();
}


//-------------------------------------------------------------------------------------------------
void App::Initialize()
{
	g_app = new App();

	StringIdSystem::Initialize();
	EventSystem::Initialize();
	g_eventSystem->SubscribeEventCallbackObjectMethod("window-resize", &App::SignalSwapChainRebuild, *g_app);

	Window::Initialize((16.f / 9.f), "Vulkan - Test");
	g_window->RegisterMessageHandler(AppMessageHandler);
	g_app->InitVulkan();
	Clock::ResetMaster();
	InputSystem::Initialize();
	JobSystem::Initialize();

	g_app->m_game = new Game();
}


//-------------------------------------------------------------------------------------------------
void App::Shutdown()
{
	SAFE_DELETE(g_app->m_game);

	JobSystem::Shutdown();
	InputSystem::Shutdown();
	g_window->UnregisterMessageHandler(AppMessageHandler);
	Window::Shutdown();

	g_eventSystem->UnsubscribeEventCallbackObjectMethod("window-resize", &App::SignalSwapChainRebuild, *g_app);
	EventSystem::Shutdown();
	StringIdSystem::Shutdown();

	SAFE_DELETE(g_app);
}


//-------------------------------------------------------------------------------------------------
void App::RunFrame()
{
	Clock::BeginMasterFrame();
	RunMessagePump();

	// Begin Frames...
	g_inputSystem->BeginFrame();
	g_eventSystem->BeginFrame();

	// Game Frame
	ProcessInput();
	Update();
	Render();

	// End Frames...
	g_inputSystem->EndFrame();
}


//-------------------------------------------------------------------------------------------------
void App::Quit()
{
	m_isQuitting = true;
}


//-------------------------------------------------------------------------------------------------
bool App::SignalSwapChainRebuild(NamedProperties& args)
{
	UNUSED(args);

	m_needSwapChainRebuild = true;
	return false;
}


//-------------------------------------------------------------------------------------------------
void App::ProcessInput()
{
	m_game->ProcessInput();
}


//-------------------------------------------------------------------------------------------------
void App::Update()
{
	m_game->Update();
}


//-------------------------------------------------------------------------------------------------
void App::Render()
{
	vkWaitForFences(m_vkLogicalDevice, 1, &m_vkInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	// Get the backbuffer
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_vkLogicalDevice, m_vkSwapChain, UINT64_MAX, m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}
	else
	{
		ASSERT_OR_DIE(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Couldn't acquire backbuffer!");
	}

	if (m_vkImagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_vkLogicalDevice, 1, &m_vkImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_vkImagesInFlight[imageIndex] = m_vkInFlightFences[m_currentFrame];

	// Specify the commands to queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_vkImageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_vkLogicalDevice, 1, &m_vkInFlightFences[m_currentFrame]);

	result = vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, m_vkInFlightFences[m_currentFrame]);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't submit commands!");

	// Present
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_vkSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_needSwapChainRebuild)
	{
		RecreateSwapChain();
	}
	else
	{
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't present!");
	}

	m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;
}


//-------------------------------------------------------------------------------------------------
bool CheckValidationLayerSupport(const std::vector<const char*> desiredLayers)
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr); // Get the number of validation layers supported

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // Request that many layers

	for (const char* layerName : desiredLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}


//-------------------------------------------------------------------------------------------------
void App::InitVulkan()
{
	CreateVulkanInstance();
	SetupVulkanDebugMessenger();
	CreateWindowSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();
}


//-------------------------------------------------------------------------------------------------
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	UNUSED(messageType);
	UNUSED(pUserData);

	const char* prefix = "";
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		prefix = "VK VERBOSE: ";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		prefix = "VK INFO: ";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		prefix = "VK WARNING: ";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		prefix = "VK ERROR: ";
		break;
	default:
		break;
	}

	DebuggerPrintf("%s%s\n", prefix, pCallbackData->pMessage);
	return VK_FALSE;
}


//-------------------------------------------------------------------------------------------------
void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& out_info)
{
	out_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	out_info.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	out_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	out_info.pfnUserCallback = VulkanDebugCallback;
	out_info.pUserData = nullptr;
}


//-------------------------------------------------------------------------------------------------
void App::CreateVulkanInstance()
{
	VkApplicationInfo appInfo{};

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Test";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "MechroEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	if (m_enableValidationLayers)
	{
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	createInfo.ppEnabledExtensionNames = enabledExtensions.data();
	createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();

	// Extension querying
	//uint32_t extensionCount = 0;
	//vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // Needed to get the number of extensions
	//std::vector<VkExtensionProperties> extensions(extensionCount);
	//vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); // Then request this many

	// Validation Layers
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	if (m_enableValidationLayers)
	{
		ASSERT_OR_DIE(CheckValidationLayerSupport(validationLayers), "Validation layer not supported!");

		createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();

		// For debugging creation/destruction
		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create instance!");
}


//-------------------------------------------------------------------------------------------------
void App::SetupVulkanDebugMessenger()
{
	if (!m_enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");
	ASSERT_OR_DIE(func != nullptr, "Failed to look up debug function!");

	VkResult result = func(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Failed to create debug messenger!");
}


//-------------------------------------------------------------------------------------------------
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};


//-------------------------------------------------------------------------------------------------
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, App* app)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int queueFamilyIndex = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (indices.IsComplete())
			break;

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = queueFamilyIndex;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, app->m_vkSurface, &presentSupport);

		if (presentSupport)
		{
			indices.presentFamily = queueFamilyIndex;
		}

		queueFamilyIndex++;
	}

	return indices;
}


//-------------------------------------------------------------------------------------------------
bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	// Get the number of extensions supported by the device
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// Get the extensions
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// For extension provided, remove it from the list of ones we need
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	// Anything still left isn't supported
	return requiredExtensions.empty();
}


//-------------------------------------------------------------------------------------------------
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


//-------------------------------------------------------------------------------------------------
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, App* app)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, app->m_vkSurface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, app->m_vkSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, app->m_vkSurface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, app->m_vkSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, app->m_vkSurface, &presentModeCount, details.presentModes.data());
	}


	return details;
}


//-------------------------------------------------------------------------------------------------
VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// Just choose SRGB color space and format
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}


//-------------------------------------------------------------------------------------------------
VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
	for (const auto& availablePresentMode : availableModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) // For triple buffering
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; // This mode is always supported
}


//-------------------------------------------------------------------------------------------------
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	IntVector2 clientDimensions = g_window->GetClientDimensions();

	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { (uint32_t)clientDimensions.x, (uint32_t)clientDimensions.y };

		actualExtent.width = Max(capabilities.minImageExtent.width, Min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = Max(capabilities.minImageExtent.height, Min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}


//-------------------------------------------------------------------------------------------------
bool IsDeviceSuitable(VkPhysicalDevice device, App* app)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Example on features
	//VkPhysicalDeviceFeatures deviceFeatures;
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = FindQueueFamilies(device, app);
	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainSupported = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, app);
		swapChainSupported = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty(); // If it at least has one format and one present mode, it's good
	}

	// For now just ensure we're using the graphics card
	return indices.IsComplete() && extensionsSupported && swapChainSupported && deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}


//-------------------------------------------------------------------------------------------------
void App::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

	ASSERT_OR_DIE(deviceCount > 0, "No devices that support Vulkan!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());


	for (const auto& device : devices)
	{
		if (IsDeviceSuitable(device, this))
		{
			m_vkPhysicalDevice = device;
			break;
		}
	}

	ASSERT_OR_DIE(m_vkPhysicalDevice != VK_NULL_HANDLE, "Could not find a suitable device!");
}


//-------------------------------------------------------------------------------------------------
void App::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_vkPhysicalDevice, this);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType =
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// No features for now
	VkPhysicalDeviceFeatures deviceFeatures{};

	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	// Create the logical device
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = 0; // Technically don't need to set validation layers here, but *should* for older implementation compatibility

	VkResult result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkLogicalDevice);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create logical device!");

	vkGetDeviceQueue(m_vkLogicalDevice, indices.graphicsFamily.value(), 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkLogicalDevice, indices.presentFamily.value(), 0, &m_vkPresentQueue);
}


//-------------------------------------------------------------------------------------------------
void App::CreateWindowSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = (HWND)g_window->GetWindowContext();
	createInfo.hinstance = GetModuleHandle(nullptr);

	VkResult result = vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &m_vkSurface);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create surface!");
}


//-------------------------------------------------------------------------------------------------
void App::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_vkPhysicalDevice, this);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0)
	{
		imageCount = Min(imageCount, swapChainSupport.capabilities.maxImageCount);
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_vkSurface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(m_vkPhysicalDevice, this);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	// We need to specify how to handle swap chain images that will be used
	// across multiple queue families.That will be the case in our application if the
	// graphics queue family is different from the presentation queue. We’ll be drawing
	// on the images in the swap chain from the graphics queue and then submitting
	// them on the presentation queue.There are two ways to handle images that are
	// accessed from multiple queues

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image is shared, no transferring needed, but slower
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Image is owned by one queue family at a time, ownership must be transferred
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}


	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Blending with other windows, we don't want that

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(m_vkLogicalDevice, &createInfo, nullptr, &m_vkSwapChain);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create swap chain!");

	// Get the images
	vkGetSwapchainImagesKHR(m_vkLogicalDevice, m_vkSwapChain, &imageCount, nullptr);
	m_vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkLogicalDevice, m_vkSwapChain, &imageCount, m_vkSwapChainImages.data());

	m_vkSwapChainImageFormat = surfaceFormat.format;
	m_vkSwapChainExtent = extent;
}


//-------------------------------------------------------------------------------------------------
void App::RecreateSwapChain()
{
	vkDeviceWaitIdle(m_vkLogicalDevice);
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateCommandPool();
	CreateCommandBuffers();

	m_needSwapChainRebuild = false;
}


//-------------------------------------------------------------------------------------------------
void App::CreateSwapChainImageViews()
{
	m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());

	for (size_t i = 0; i < m_vkSwapChainImages.size(); ++i)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_vkSwapChainImages[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_vkSwapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(m_vkLogicalDevice, &createInfo, nullptr, &m_vkSwapChainImageViews[i]);
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create swap chain image views!");
	}
}


//-------------------------------------------------------------------------------------------------
void App::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_vkSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the buffer at the start of the pass
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the data at the end
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil yet
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // No stencil yet
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // Index into the renderPassInfo.pAttachments array
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef; // This array is the one indexed in the fragment shader, with layout(location = 0)out vec4 outColor

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	// "built-in" dependencies
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subpassDependency;

	VkResult result = vkCreateRenderPass(m_vkLogicalDevice, &renderPassInfo, nullptr, &m_vkRenderPass);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create render pass!");
}


//-------------------------------------------------------------------------------------------------
VkShaderModule CreateShaderModule(const char* byteCode, size_t size, VkDevice vkDevice)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode);

	VkShaderModule vkShaderModule;
	VkResult result = vkCreateShaderModule(vkDevice, &createInfo, nullptr, &vkShaderModule);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create shader module!");

	return vkShaderModule;
}


//-------------------------------------------------------------------------------------------------
void App::CreateGraphicsPipeline()
{
	File vertFile;
	if (vertFile.Open("Data/Shader/vert.spv", "rb"))
	{
		vertFile.LoadFileToMemory();
		vertFile.Close();
	}

	File fragFile;
	if (fragFile.Open("Data/Shader/frag.spv", "rb"))
	{
		fragFile.LoadFileToMemory();
		fragFile.Close();
	}

	ASSERT_OR_DIE(vertFile.GetSize() > 0, "Couldn't load vertex file!");
	ASSERT_OR_DIE(fragFile.GetSize() > 0, "Couldn't load fragment file!");


	// We don't need to keep the shader modules around, they're only needed for the pipeline creation
	// So they can be local and destroyed immediately after creation!
	VkShaderModule vertModule = CreateShaderModule(vertFile.GetData(), vertFile.GetSize(), m_vkLogicalDevice);
	VkShaderModule fragModule = CreateShaderModule(fragFile.GetData(), fragFile.GetSize(), m_vkLogicalDevice);

	// Create the shader stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertModule;
	vertShaderStageInfo.pName = "main"; // Name of entry point function

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Vertex data is hard coded in the shader, so no vertex input needed yet
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_vkSwapChainExtent.width;
	viewport.height = (float)m_vkSwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_vkSwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE; // If true, won't discard outside depth value but instead clamp, requires a logical device feature enabled, good for shadow maps
	rasterizer.rasterizerDiscardEnable = VK_FALSE; // If true, geo never goes to the rasterizer stage, basically disabling output to framebuffer
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// MSAA, disable for now
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Skipping depth stencil for now

	// Blend state, *per framebuffer*
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	// Global
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// For adjusting some things without recreating the pipeline
	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Pipeline layout (uniforms)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	VkResult layoutResult = vkCreatePipelineLayout(m_vkLogicalDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout);
	ASSERT_OR_DIE(layoutResult == VK_SUCCESS, "Cannot create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	// Fixed-function stages
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = m_vkPipelineLayout;
	pipelineInfo.renderPass = m_vkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	VkResult pipelineResult = vkCreateGraphicsPipelines(m_vkLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkGraphicsPipeline);
	ASSERT_OR_DIE(pipelineResult == VK_SUCCESS, "Can't create graphics pipeline!");

	vkDestroyShaderModule(m_vkLogicalDevice, vertModule, nullptr);
	vkDestroyShaderModule(m_vkLogicalDevice, fragModule, nullptr);
}


//-------------------------------------------------------------------------------------------------
void App::CreateFrameBuffers()
{
	m_vkFramebuffers.resize(m_vkSwapChainImageViews.size());

	for (size_t i = 0; i < m_vkSwapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { m_vkSwapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_vkRenderPass; // For compatibility
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_vkSwapChainExtent.width;
		framebufferInfo.height = m_vkSwapChainExtent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(m_vkLogicalDevice, &framebufferInfo, nullptr, &m_vkFramebuffers[i]);
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create framebuffer!");
	}
}


//-------------------------------------------------------------------------------------------------
void App::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_vkPhysicalDevice, this);
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; // Optional

	VkResult result = vkCreateCommandPool(m_vkLogicalDevice, &poolInfo, nullptr, &m_vkCommandPool);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create command pool!");
}


//-------------------------------------------------------------------------------------------------
void App::CreateCommandBuffers()
{
	m_vkCommandBuffers.resize(m_vkFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

	VkResult result = vkAllocateCommandBuffers(m_vkLogicalDevice, &allocInfo, m_vkCommandBuffers.data());
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't allocate command buffers!");

	// Do command buffer recording
	for (size_t i = 0; i < m_vkCommandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		result = vkBeginCommandBuffer(m_vkCommandBuffers[i], &beginInfo);
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't start recording command buffer!");

		// Render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vkRenderPass;
		renderPassInfo.framebuffer = m_vkFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_vkSwapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);

		vkCmdDraw(m_vkCommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_vkCommandBuffers[i]);

		result = vkEndCommandBuffer(m_vkCommandBuffers[i]);
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't record command!");
	}
}


//-------------------------------------------------------------------------------------------------
void App::CreateSyncObjects()
{
	m_vkImageAvailableSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
	m_vkRenderFinishedSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkResult result = vkCreateSemaphore(m_vkLogicalDevice, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[i]);
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create semaphore!");

		result = vkCreateSemaphore(m_vkLogicalDevice, &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[i]);
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create semaphore!");
	}

	// Fences
	m_vkInFlightFences.resize(m_MAX_FRAMES_IN_FLIGHT);
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkResult result = vkCreateFence(m_vkLogicalDevice, &fenceInfo, nullptr, &m_vkInFlightFences[i]);
		ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create fence!");
	}

	m_vkImagesInFlight.resize(m_vkSwapChainImages.size());
}


//-------------------------------------------------------------------------------------------------
void App::ShutdownVulkan()
{
	CleanUpSwapChain();

	for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroyFence(m_vkLogicalDevice, m_vkInFlightFences[i], nullptr);
	}

	for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_vkLogicalDevice, m_vkRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_vkLogicalDevice, m_vkImageAvailableSemaphores[i], nullptr);
	}


	vkDestroyCommandPool(m_vkLogicalDevice, m_vkCommandPool, nullptr);

	vkDestroyDevice(m_vkLogicalDevice, nullptr);

	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

	if (m_enableValidationLayers)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		ASSERT_OR_DIE(func != nullptr, "Failed to look up debug function!");
		func(m_vkInstance, m_vkDebugMessenger, nullptr);
	}

	vkDestroyInstance(m_vkInstance, nullptr);
}


//-------------------------------------------------------------------------------------------------
void App::CleanUpSwapChain()
{
	vkDeviceWaitIdle(m_vkLogicalDevice);

	vkFreeCommandBuffers(m_vkLogicalDevice, m_vkCommandPool, (uint32_t)m_vkCommandBuffers.size(), m_vkCommandBuffers.data());

	for (VkFramebuffer framebuffer : m_vkFramebuffers)
	{
		vkDestroyFramebuffer(m_vkLogicalDevice, framebuffer, nullptr);
	}

	vkDestroyPipeline(m_vkLogicalDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_vkLogicalDevice, m_vkPipelineLayout, nullptr);
	vkDestroyRenderPass(m_vkLogicalDevice, m_vkRenderPass, nullptr);

	for (VkImageView view : m_vkSwapChainImageViews)
	{
		vkDestroyImageView(m_vkLogicalDevice, view, nullptr);
	}
	m_vkSwapChainImageViews.clear();

	vkDestroySwapchainKHR(m_vkLogicalDevice, m_vkSwapChain, nullptr);
	m_vkSwapChainImages.clear();
}
