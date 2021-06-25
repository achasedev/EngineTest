///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: November 29th, 2019
/// Description: Interface/Manager between Game code and Engine code
///--------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "ThirdParty/Vulkan/Include/vulkan/vulkan.h"
#include <vector>

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
class Game;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
class App
{
public:
	//-----Public Methods-----

	App();
	~App();

	static void Initialize();
	static void Shutdown();

	void RunFrame();
	void Quit();

	bool IsQuitting() const { return m_isQuitting; }


private:
	//-----Private Methods-----

	// "One Frame"
	void ProcessInput();
	void Update();
	void Render();

	void InitVulkan();
	void CreateVulkanInstance();
	void SetupVulkanDebugMessenger();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateWindowSurface();
	void CreateSwapChain();
	void CreateSwapChainImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();

	void ShutdownVulkan();


private:
	//-----Private Data-----

	bool	m_isQuitting = false;
	Game*	m_game = nullptr;

public:

	// Vulkan
	VkInstance m_vkInstance;
	bool m_enableValidationLayers = true;
	VkDebugUtilsMessengerEXT m_vkDebugMessenger;
	VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_vkLogicalDevice;
	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;
	VkSurfaceKHR m_vkSurface;
	VkSwapchainKHR m_vkSwapChain;
	std::vector<VkImage> m_vkSwapChainImages;
	std::vector<VkImageView> m_vkSwapChainImageViews;
	VkFormat m_vkSwapChainImageFormat;
	VkExtent2D m_vkSwapChainExtent;
	VkRenderPass m_vkRenderPass;
	VkPipelineLayout m_vkPipelineLayout;	VkPipeline m_vkGraphicsPipeline;};


///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
