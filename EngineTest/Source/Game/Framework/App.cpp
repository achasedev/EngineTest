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
	m_game->Render();
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
	return VK_FALSE;}

//-------------------------------------------------------------------------------------------------
void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& out_info)
{
	out_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	out_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
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
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
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

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");
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
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, app->m_vkSurface, &presentModeCount, details.presentModes.data());	}

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
		//VkExtent2D actualExtent = { (uint32_t) clientDimensions.x, (uint32_t) clientDimensions.y };
		VkExtent2D actualExtent = { clientDimensions.x, clientDimensions.y };
	
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

	VkResult result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create logical device!");

	vkGetDeviceQueue(m_vkDevice, indices.graphicsFamily.value(), 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, indices.presentFamily.value(), 0, &m_vkPresentQueue);
}


//-------------------------------------------------------------------------------------------------
void App::CreateWindowSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = (HWND) g_window->GetWindowContext();
	createInfo.hinstance = GetModuleHandle(nullptr);

	VkResult result = vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &m_vkSurface);
	ASSERT_OR_DIE(result == VK_SUCCESS, "Couldn't create surface!");
}


//-------------------------------------------------------------------------------------------------
void App::CreateSwapChain()
{

}


//-------------------------------------------------------------------------------------------------
void App::ShutdownVulkan()
{
	vkDestroyDevice(m_vkDevice, nullptr);

	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

	if (m_enableValidationLayers)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		ASSERT_OR_DIE(func != nullptr, "Failed to look up debug function!");
		func(m_vkInstance, m_vkDebugMessenger, nullptr);
	}

	vkDestroyInstance(m_vkInstance, nullptr);
}
