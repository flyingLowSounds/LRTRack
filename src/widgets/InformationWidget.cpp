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
**    Copyright 2017-2019 by Patrick Lindenberg / LRT                  **
**                                                                     **
**    For Redistribution and use in source and binary forms,           **
**    with or without modification please see LICENSE.                 **
**                                                                     **
\*                                                                     */
#include "../LRComponents.hpp"
#include "../LindenbergResearch.hpp"

using namespace lrt;

#ifdef LRT_DEBUG

void InformationWidget::draw(const Widget::DrawArgs &args) {
    nvgSave(args.vg);

    nvgFontSize(args.vg, 12);
    nvgFontFaceId(args.vg, statsttf->handle);
    nvgTextLetterSpacing(args.vg, 0.6);

    string fps = stringf("FPS    => %05.2f", APP->window->stats.fps);
    string duration = stringf("TIME   => %05.2fms", APP->window->stats.duration * 1000);
    string gpu = stringf("GPU    => %05.2f%%", APP->window->stats.gpu);
    string frame = stringf("FRAME  => %07d", APP->window->frame);

    float dist = 17;
    float offs = 95;

    string ver = stringf("RACK AUDIO MODULES V%s", pluginInstance->version.c_str());

    nvgTextBox(args.vg, 60, 14, box.size.x - 40, ver.c_str(), nullptr);

    nvgFillColor(args.vg, nvgRGBAf(0.19, 0.99, 0.99, 1.0));
    nvgTextBox(args.vg, 10, 25 + dist, box.size.x - 40, "(c) 2017-2019 Lindenberg Research.", nullptr);
    nvgTextBox(args.vg, 10, 25 + dist * 2, box.size.x - 40, "All rights reserved.", nullptr);


    if (APP->window->frame % 50 < 25)
        nvgTextBox(args.vg, 10, 25 + dist * 3, box.size.x - 40, "OPERATING IN DEBUGGER MODE.", nullptr);
    else
        nvgTextBox(args.vg, 10, 25 + dist * 3, box.size.x - 40, "OPERATING IN DEBUGGER MODE. _", nullptr);
    //  nvgFill(args.vg);

    nvgFillColor(args.vg, nvgRGBAf(0.99, 0.99, 0.99, 1.0));
    nvgTextBox(args.vg, 10, offs + dist * 1, box.size.x - 40, fps.c_str(), nullptr);
    nvgTextBox(args.vg, 10, offs + dist * 2, box.size.x - 40, duration.c_str(), nullptr);
    nvgTextBox(args.vg, 10, offs + dist * 3, box.size.x - 40, gpu.c_str(), nullptr);
    nvgTextBox(args.vg, 10, offs + dist * 4, box.size.x - 40, frame.c_str(), nullptr);
    //  nvgFill(args.vg);

    double c = box.size.x / APP->window->stats.curve.size();
    double avrg = 0;
    double h = 0, norm = 0;

    for (unsigned long i = 0; i < APP->window->stats.curve.size(); i++) {
        h = APP->window->stats.curve[i];
        norm = tanh(h / 100);

        avrg += h;

        nvgBeginPath(args.vg);
        nvgFillColor(args.vg, nvgRGBAf(norm, 1 - norm, 0, 1));
        nvgRect(args.vg, i * c - 1, 320 - norm * 130, c + 1, norm * 130);
        nvgClosePath(args.vg);
        nvgFill(args.vg);
    }

    avrg /= APP->window->stats.curve.size();


    string avrgs = stringf("%5.2f%%", avrg);

    norm = tanh(avrg / 100);

    nvgFontSize(args.vg, 10);
    nvgFillColor(args.vg, nvgRGBAf(norm, 1 - norm, 0, 1));
    nvgTextBox(args.vg, box.size.x / 2, 320 - 15 - norm * 130, box.size.x - 40, avrgs.c_str(), nullptr);

    nvgBeginPath(args.vg);
    nvgFillColor(args.vg, nvgRGBAf(0, 1, 1, 0.9));
    nvgRect(args.vg, 0, 320 - norm * 130, box.size.x, 2);
    nvgClosePath(args.vg);
    nvgFill(args.vg);

    nvgRestore(args.vg);
}


InformationWidget::InformationWidget() {
    statsttf = APP->window->loadFont(asset::plugin(pluginInstance, "res/Px437_AMI_BIOS-2y.ttf"));

    APP->window->stats.interval = 2;
}


#endif
