///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: November 29th, 2019
/// Description: Interface/Manager between Game code and Engine code
///--------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Render/VulkanCommon.h"
#include "Engine/Render/Vertex.h"
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
class NamedProperties;

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
	bool SignalSwapChainRebuild(NamedProperties& args);


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
	void RecreateSwapChain();
	void CreateSwapChainImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void ShutdownVulkan();
	void CleanUpSwapChain();


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
	VkPipelineLayout m_vkPipelineLayout;	VkPipeline m_vkGraphicsPipeline;	std::vector<VkFramebuffer> m_vkFramebuffers;	VkCommandPool m_vkCommandPool;	std::vector<VkCommandBuffer> m_vkCommandBuffers;	std::vector<VkSemaphore> m_vkImageAvailableSemaphores;	std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;	std::vector<VkFence> m_vkInFlightFences;	std::vector<VkFence> m_vkImagesInFlight;	const int m_MAX_FRAMES_IN_FLIGHT = 2;	size_t m_currentFrame = 0;	bool m_needSwapChainRebuild = false;	Vertex3D_PC m_vertices[3];};


///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
