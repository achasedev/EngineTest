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
#include "Engine/Time/Clock.h"
#include "Engine/Utility/StringID.h"

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
#ifdef NDEBUG
	m_enableValidationLayers = false;
#endif

	CreateVulkanInstance();
	SetupVulkanDebugMessenger();
}


//-------------------------------------------------------------------------------------------------
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
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
void App::ShutdownVulkan()
{
	if (m_enableValidationLayers)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		ASSERT_OR_DIE(func != nullptr, "Failed to look up debug function!");
		func(m_vkInstance, m_vkDebugMessenger, nullptr);
	}

	vkDestroyInstance(m_vkInstance, nullptr);
}
