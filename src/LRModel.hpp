/*                                                                     *\
**       __   ___  ______                                              **
**      / /  / _ \/_  __/                                              **
**     / /__/ , _/ / /    Lindenberg                                   **
**    /____/_/|_| /_/  Research Tec.                                   **
**                                                                     **
**                                                                     **
**	  https://github.com/lindenbergresearch/LRTRack	                   **
**    heapdump@icloud.com                                              **
**		                                                               **
**    Sound Modules for VCV Rack                                       **
**    Copyright 2017/2018 by Patrick Lindenberg / LRT                  **
**                                                                     **
**    For Redistribution and use in source and binary forms,           **
**    with or without modification please see LICENSE.                 **
**                                                                     **
\*                                                                     */

#pragma once

#include "rack.hpp"
#include "asset.hpp"
#include "widgets.hpp"
#include "LRComponents.hpp"


static const char *const JSON_GESTALT_KEY = "gestaltID";
static const char *const JSON_GRADIENT_KEY = "gradient";
static const char *const JSON_PATINA_KEY = "patina";


namespace lrt {

using std::string;
using std::vector;


static const string STR_CHECKMARK_UNICODE = "✔";


/**
 * @brief Standard LR Module definition
 */
struct LRModule : public Module {
    LRGestalt *gestalt = nullptr;

    /**
     * @brief Default constructor derived from rack
     * @param numParams
     * @param numInputs
     * @param numOutputs
     * @param numLights
     */
    explicit LRModule(int numParams, int numInputs, int numOutputs, int numLights);
};


/**
 * @brief Standard LR ModuleWidget definition
 */
struct LRModuleWidget : ModuleWidget {

    /* standard module panel */
    LRPanel *panel = nullptr;

    /* Gestalt ID and UI settings */
    LRGestalt gestalt = DARK;   // DARK == default
    bool gradient = true;       // gradient used at panel
    bool patina = false;        // patina used at panel

    /**
     * @brief Default constructor derived from rack
     * @param module LRModule instance
     */
    explicit LRModuleWidget(LRModule *module) : ModuleWidget(module) {
        // pass gestalt pointer to module, to point to the origin at the widget
        module->gestalt = &gestalt;
    }


    /**
     * @brief Represents an Item for selecting the gestalt
     */
    struct GestaltItem : MenuItem {
        LRGestalt gestaltM;
        LRModuleWidget *widget;


        GestaltItem(LRGestalt gestaltM, LRModuleWidget *widget) : gestaltM(gestaltM), widget(widget) {}


        void onAction(EventAction &e) override {
            if (widget != nullptr) {
                // set new global gestalt to current ID of selected menu item
                widget->gestalt = gestaltM;
            }
        }


        void step() override {
            rightText = (widget->gestalt == gestaltM) ? STR_CHECKMARK_UNICODE : "";
        }
    };


    /**
     * @brief Represents a gradient select item
     */
    struct GradientItem : MenuItem {
        LRPanel *panel;


        explicit GradientItem(LRPanel *panel) : panel(panel) {}


        void onAction(EventAction &e) override {
            if (panel != nullptr) {
                panel->setGradientVariant(true);
                panel->dirty = true;
            }
        }


        void step() override {
            rightText = *panel->gradient ? STR_CHECKMARK_UNICODE : "";
        }
    };


    /**
     * @brief Represents a gradient select item
     */
    struct PatinaItem : MenuItem {
        LRPanel *panel;


        explicit PatinaItem(LRPanel *panel) : panel(panel) {}


        void onAction(EventAction &e) override {
            if (panel != nullptr) {
                panel->setPatina(!*panel->patina); // invert flag on trigger
                panel->dirty = true;
            }
        }


        void step() override {
            rightText = *panel->patina ? STR_CHECKMARK_UNICODE : "";
        }
    };


    /**
     * @brief Create standard menu for all modules
     * @return
     */
    Menu *createContextMenu() override;

    json_t *toJson() override;
    void fromJson(json_t *rootJ) override;
    void randomize() override;
};


}