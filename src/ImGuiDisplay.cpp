#include "ImguiDisplay.h"

#include <FmGui.hpp>
#include <functional>
#include <imgui.h>

std::optional<ImguiDisplay> ImguiDisplay::display;

void ImguiDisplay::AddImguiItem( const std::string& menu, const std::string& name, std::function<void( lua_State*, std::string item, bool*)>&& imgui_function )
{
    if ( ! display.has_value() )
    {
        printf( "No Imgui\n" );
        return;
    }

    Menu& imgui_menu = display->menus[menu];
    imgui_menu.items.emplace_back();

    MenuItem& imgui_menu_item = imgui_menu.items.back();
    imgui_menu_item.imgui_function = std::move( imgui_function );
    imgui_menu_item.name = name;
}

void ImguiDisplay::DisplayHook()
{
    if ( display )
        display->Display();
}


void ImguiDisplay::RefreshDisplay( lua_State* L )
{
    if ( display )
        display->Refresh(L);
}

void ImguiDisplay::InputHook( UINT msg, WPARAM w_param, LPARAM l_param )
{
    if ( display )
        display->Input( msg, w_param, l_param );
}

ImguiDisplay::ImguiDisplay()
{
    FmGuiConfig config;
    config.imGuiStyle = FmGuiStyle::CLASSIC;

    if ( ! FmGui::StartupHook( config ) )
    {
        printf( "FmGui::StartupHook failed.\n" );
    }
    else
    {
        printf( "D3D11 Context: %s\n", FmGui::AddressDump().c_str() );

        FmGui::SetInputRoutinePtr( InputHook );
        FmGui::SetRoutinePtr( DisplayHook );
        FmGui::SetWidgetVisibility( true );
    }
}

ImguiDisplay::~ImguiDisplay()
{
    if ( ! FmGui::ShutdownHook() )
    {
        printf( "FmGui::ShutdownHook failed.\n" );
    }
}

void ImguiDisplay::Refresh( lua_State* L )
{
    if ( error )
        return;
    
    if ( menus.empty() )
        return;

    std::unique_lock lock( command_mtx );
    commands[L].clear();

    /**/

    for ( auto& [menu_name, menu] : menus )
    {
        for ( auto& menu_item : menu.items )
        {

            if ( ! menu_item.visible )
            {
                continue;
            }

            std::string path = menu_name + "/" + menu_item.name;
            
            //if ( ImGui::Begin( path.c_str(), &menu_item.visible ) )
            //{
            if ( menu_item.imgui_function )
                menu_item.imgui_function( L, std::move( path ), &menu_item.visible );
                //ImGui::End();
            //}
        }
    }
}

void ImguiDisplay::Display()
{
    if ( error )
        return;

    std::unique_lock lock( command_mtx );
    if ( ImGui::BeginMainMenuBar() )
    {
        for ( auto& [menu_name, menu] : menus )
        {
            if ( ! ImGui::BeginMenu( menu_name.c_str() ) )
            {
                continue;
            }

            for ( auto& menu_item : menu.items )
            {
                if ( ImGui::MenuItem( menu_item.name.c_str() ) )
                {
                    menu_item.visible = true;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    int allowed_depth = 0;

    for ( auto& [L, L_commands] : commands )
    {
        for ( auto& c : L_commands )
        {
            if ( c.depth <= allowed_depth )
            {
                if ( c.command( c.control ) )
                {
                    allowed_depth = c.depth + 1;
                }
                else
                {
                    allowed_depth = c.depth;
                }
            }
        }
    }
}

void ImguiDisplay::Input( UINT msg, WPARAM w_param, LPARAM l_param )
{

}