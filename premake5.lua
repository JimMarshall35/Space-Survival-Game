local ProjectName = _ARGS[1]

workspace(ProjectName)
	architecture "x64"
	startproject "Editor"
	configurations { "Debug", "Release" }

project "Editor"
	location "Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	systemversion "latest"

	dependson
	{
		"Engine",
		"glfw"
	}

	targetdir "Bin/Editor/%{cfg.buildcfg}/%{cfg.platform}"
	objdir "Bin/Intermediate/Editor/%{cfg.buildcfg}/%{cfg.platform}"

	files
	{
		"%{prj.name}/include/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/include"	
	}
	
	externalincludedirs
	{
		"%{prj.name}/../Engine/include",
		"vendor\\glfw\\include",
		"vendor\\glew\\include"
	}

	libdirs
	{
		"Bin\\Engine\\%{cfg.buildcfg}\\%{cfg.platform}",
		"vendor\\glfw\\build\\src\\%{cfg.buildcfg}",
		"vendor\\glew\\lib\\Release\\x64"
	}

	links
	{
		"Engine.lib",
		"glfw3.lib",
		"glew32.lib"
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
		runtime "Debug"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
		runtime "Release"
		
		
project "Engine"
	location "Engine"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	systemversion "latest"
	defines { "BUILD_DLL" }
	
	dependson
	{
		"glfw"
	}
	
	targetdir "Bin/Engine/%{cfg.buildcfg}/%{cfg.platform}"
	objdir "Bin/Intermediate/Engine/%{cfg.buildcfg}/%{cfg.platform}"
	
	files
	{
		"%{prj.name}/include/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/include"
	}

	externalincludedirs
	{
		"vendor\\glfw\\include",
		"vendor\\glew\\include"
	}

	libdirs
	{
		"vendor\\glfw\\build\\src\\%{cfg.buildcfg}",
		"vendor\\glew\\lib\\Release\\x64"
	}
	
	links
	{
		"opengl32.lib",
		"glew32.lib",
		"glfw3.lib"
	}

	postbuildcommands
	{
		"{MKDIR} \"%{wks.location}Bin/Editor/%{cfg.buildcfg}/%{cfg.platform}\"",
		"{COPYFILE} \"%{wks.location}Bin/Engine/%{cfg.buildcfg}/%{cfg.platform}/%{cfg.buildtarget.basename}%{cfg.buildtarget.extension}\" \"%{wks.location}Bin/Editor/%{cfg.buildcfg}/%{cfg.platform}\"",
		"rd /s /q $(SolutionDir)x64"
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
		runtime "Debug"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
		runtime "Release"

externalproject "glfw"
	location ("%{wks.location}" .. ProjectName .. "\\vendor\\glfw\\build\\src")
	uuid "F70CFA3B-BFC4-3B3F-B758-519FB418430D"
	kind "StaticLib"
	language "C++"
	
externalproject "ZERO_CHECK"
	location ("%{wks.location}" .. ProjectName .. "\\vendor\\glfw\\build")
	uuid "FC96CD67-3228-3471-843D-D7756AE336C1"
	kind "None"
	
newaction {
	trigger = "clean",
	description = "clean the software",
	execute = function ()
		print("Cleaning solution...")
		
		-- Solution files
		for _, slnFile in ipairs(os.matchfiles("*.sln")) do
            os.remove(slnFile)
        end
		
		-- Specific files with extensions
        local filesToDelete = {
            "Editor/Editor.vcxproj",
            "Engine/Engine.vcxproj",
            "Engine/Engine.vcxproj.filters"
            -- Add more file paths here
        }
		for _, filePath in ipairs(filesToDelete) do
            os.remove(filePath)
        end
		
		-- Specific directories
        local directoriesToDelete = {
			"Bin",
            "vendor/glfw/build"			
            -- Add more file paths here
        }
		for _, directoryPath in ipairs(directoriesToDelete) do
            if os.isdir(directoryPath) then
                os.rmdir(directoryPath)
            end
        end
		
		print("Done cleaning.")
	end
}