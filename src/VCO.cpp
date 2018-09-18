#include "dsp/Oscillator.hpp"
#include "LindenbergResearch.hpp"

using namespace rack;
using namespace lrt;


struct VCO : Module {
    enum ParamIds {
        FREQUENCY_PARAM,
        OCTAVE_PARAM,
        FM_CV_PARAM,
        PW_CV_PARAM,
        SAW_PARAM,
        PULSE_PARAM,
        SINE_PARAM,
        TRI_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        VOCT1_INPUT,
        FM_CV_INPUT,
        PW_CV_INPUT,
        VOCT2_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SAW_OUTPUT,
        PULSE_OUTPUT,
        SINE_OUTPUT,
        TRI_OUTPUT,
        NOISE_OUTPUT,
        MIX_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LFO_LIGHT,
        NUM_LIGHTS
    };

    dsp::DSPBLOscillator *osc = new dsp::DSPBLOscillator(engineGetSampleRate());
    LRLCDWidget *lcd = new LRLCDWidget(nvgRGBAf(0.0, 0.1, 0.1, 1.0), 10, "%00004.3f Hz", LRLCDWidget::NUMERIC);
    LRAlternateBigLight *frqKnob = NULL;

    SVGWidget *patina;
    LRPanel *panel, *panelAged;
    bool aged = true;


    VCO() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}


    json_t *toJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "aged", json_boolean(aged));
        return rootJ;
    }


    void fromJson(json_t *rootJ) override {
        json_t *agedJ = json_object_get(rootJ, "aged");
        if (agedJ)
            aged = json_boolean_value(agedJ);

        updateComponents();
    }


    void onRandomize() override;
    void updateComponents();

    void step() override;
    void onSampleRateChange() override;
};


void VCO::step() {
    Module::step();

    float fm = clamp(inputs[FM_CV_INPUT].value, -CV_BOUNDS, CV_BOUNDS) * 0.4f * quadraticBipolar(params[FM_CV_PARAM].value);
    float tune = params[FREQUENCY_PARAM].value;
    float pw;

    if (inputs[PW_CV_INPUT].active) {
        pw = clamp(inputs[PW_CV_INPUT].value, -CV_BOUNDS, CV_BOUNDS) * 0.6f * quadraticBipolar(params[PW_CV_PARAM].value / 2.f) + 1;
        pw = clamp(pw, 0.01, 1.99);
    } else {
        pw = params[PW_CV_PARAM].value * 0.99f + 1;
    }

    if (frqKnob != NULL) {
        frqKnob->setIndicatorActive(inputs[FM_CV_INPUT].active);
        frqKnob->setIndicatorValue((params[FREQUENCY_PARAM].value + 1) / 2 + (fm / 2));
    }

    osc->setInputs(inputs[VOCT1_INPUT].value, inputs[VOCT2_INPUT].value, fm, tune, params[OCTAVE_PARAM].value);
    osc->setPulseWidth(pw);

    osc->process();

    outputs[SAW_OUTPUT].value = osc->getSawWave();
    outputs[PULSE_OUTPUT].value = osc->getPulseWave();
    outputs[SINE_OUTPUT].value = osc->getSineWave();
    outputs[TRI_OUTPUT].value = osc->getTriWave();
    outputs[NOISE_OUTPUT].value = osc->getNoise();


    if (outputs[MIX_OUTPUT].active) {
        float mix = 0.f;

        mix += osc->getSawWave() * params[SAW_PARAM].value;
        mix += osc->getPulseWave() * params[PULSE_PARAM].value;
        mix += osc->getSineWave() * params[SINE_PARAM].value;
        mix += osc->getTriWave() * params[TRI_PARAM].value;

        outputs[MIX_OUTPUT].value = mix;
    }

    /* for LFO mode */
    if (osc->isLFO())
        lights[LFO_LIGHT].setBrightnessSmooth(osc->getSineWave() / 10.f + 0.3f);
    else lights[LFO_LIGHT].value = 0.f;

    lcd->active = osc->isLFO();
    lcd->value = osc->getFrequency();
}


void VCO::updateComponents() {
    patina->visible = aged;
    panelAged->visible = aged;
    panel->visible = !aged;

    panelAged->dirty = true;
    panel->dirty = true;
}


void VCO::onSampleRateChange() {
    Module::onSampleRateChange();
    osc->updateSampleRate(engineGetSampleRate());
}


void VCO::onRandomize() {
    Module::randomize();
    patina->box.pos = Vec(-randomUniform() * 1000, -randomUniform() * 200);

    updateComponents();
}


/**
 * @brief Woldemar VCO Widget
 */
struct VCOWidget : LRModuleWidget {
    VCOWidget(VCO *module);
    void appendContextMenu(Menu *menu) override;
};


VCOWidget::VCOWidget(VCO *module) : LRModuleWidget(module) {
    panel = new LRPanel(-10, -10);
    panel->setBackground(SVG::load(assetPlugin(plugin, "res/panels/Woldemar.svg")));
    addChild(panel);

    module->panel = panel;

    module->panelAged = new LRPanel(-10, -10);
    module->panelAged->setBackground(SVG::load(assetPlugin(plugin, "res/panels/WoldemarAged.svg")));
    module->panelAged->visible = false;
    addChild(module->panelAged);

    box.size = panel->box.size;


    module->patina = new SVGWidget();
    module->patina->setSVG(SVG::load(assetPlugin(plugin, "res/panels/LaikaPatina.svg")));
    module->panelAged->addChild(module->patina);

    module->patina->box.pos = Vec(-randomUniform() * 1000, -randomUniform() * 200);

    panel->setInner(nvgRGBAf(0.3, 0.3, 0.f, 0.09f));
    panel->setOuter(nvgRGBAf(0.f, 0.f, 0.f, 0.7f));

    module->panelAged->setInner(nvgRGBAf(0.5, 0.5, 0.f, 0.1f));
    module->panelAged->setOuter(nvgRGBAf(0.f, 0.f, 0.f, 0.73f));


    // **** SETUP LCD ********
    module->lcd->box.pos = Vec(24, 242);
    module->lcd->format = "%00004.3f Hz";
    addChild(module->lcd);
    // **** SETUP LCD ********


    // ***** SCREWS **********
    addChild(Widget::create<AlternateScrewLight>(Vec(15, 1)));
    addChild(Widget::create<AlternateScrewLight>(Vec(box.size.x - 30, 1)));
    addChild(Widget::create<AlternateScrewLight>(Vec(15, 366)));
    addChild(Widget::create<AlternateScrewLight>(Vec(box.size.x - 30, 366)));
    // ***** SCREWS **********


    // ***** MAIN KNOBS ******
    module->frqKnob = LRKnob::create<LRAlternateBigLight>(Vec(126.0, 64.7), module, VCO::FREQUENCY_PARAM, -1.f, 1.f, 0.f);

    addParam(module->frqKnob);
    addParam(ParamWidget::create<LRAlternateToggleKnobLight>(Vec(134.6, 171.9), module, VCO::OCTAVE_PARAM, -4.f, 3.f, 0.f));

    addParam(ParamWidget::create<LRAlternateSmallLight>(Vec(69.5, 122), module, VCO::FM_CV_PARAM, -1.f, 1.f, 0.f));
    addParam(ParamWidget::create<LRAlternateSmallLight>(Vec(69.5, 175), module, VCO::PW_CV_PARAM, -1, 1, 0.f));


    addParam(ParamWidget::create<LRAlternateSmallLight>(Vec(22.8, 270.1), module, VCO::SAW_PARAM, -1.f, 1.f, 0.f));
    addParam(ParamWidget::create<LRAlternateSmallLight>(Vec(58.3, 270.1), module, VCO::PULSE_PARAM, -1.f, 1.f, 0.f));
    addParam(ParamWidget::create<LRAlternateSmallLight>(Vec(93.1, 270.1), module, VCO::SINE_PARAM, -1.f, 1.f, 0.f));
    addParam(ParamWidget::create<LRAlternateSmallLight>(Vec(128.1, 270.1), module, VCO::TRI_PARAM, -1.f, 1.f, 0.f));
    // ***** MAIN KNOBS ******


    // ***** INPUTS **********
    addInput(Port::create<LRIOPortCLight>(Vec(20.8, 67.9), Port::INPUT, module, VCO::VOCT1_INPUT));
    addInput(Port::create<LRIOPortCLight>(Vec(68.0, 67.9), Port::INPUT, module, VCO::VOCT2_INPUT));
    addInput(Port::create<LRIOPortCLight>(Vec(20.8, 121.5), Port::INPUT, module, VCO::FM_CV_INPUT));
    addInput(Port::create<LRIOPortCLight>(Vec(20.8, 174.8), Port::INPUT, module, VCO::PW_CV_INPUT));
    // ***** INPUTS **********


    // ***** OUTPUTS *********
    addOutput(Port::create<LRIOPortBLight>(Vec(21, 305.8), Port::OUTPUT, module, VCO::SAW_OUTPUT));
    addOutput(Port::create<LRIOPortBLight>(Vec(56.8, 305.8), Port::OUTPUT, module, VCO::PULSE_OUTPUT));
    addOutput(Port::create<LRIOPortBLight>(Vec(91.6, 305.8), Port::OUTPUT, module, VCO::SINE_OUTPUT));
    addOutput(Port::create<LRIOPortBLight>(Vec(126.6, 305.8), Port::OUTPUT, module, VCO::TRI_OUTPUT));
    addOutput(Port::create<LRIOPortBLight>(Vec(162.0, 305.8), Port::OUTPUT, module, VCO::NOISE_OUTPUT));
    addOutput(Port::create<LRIOPortBLight>(Vec(162.0, 269.1), Port::OUTPUT, module, VCO::MIX_OUTPUT));
    // ***** OUTPUTS *********


    // ***** LIGHTS **********
    addChild(ModuleLightWidget::create<LRLight>(Vec(181.8, 210), module, VCO::LFO_LIGHT));
    // ***** LIGHTS **********
}


struct VCOAged : MenuItem {
    VCO *vco;


    void onAction(EventAction &e) override {
        if (vco->aged) {
            vco->aged = false;
        } else {
            vco->aged = true;
        }

        vco->updateComponents();
    }


    void step() override {
        rightText = CHECKMARK(vco->aged);
    }
};


void VCOWidget::appendContextMenu(Menu *menu) {
    menu->addChild(MenuEntry::create());

    VCO *vco = dynamic_cast<VCO *>(module);
    assert(vco);

    VCOAged *mergeItemAged = MenuItem::create<VCOAged>("Use aged look");
    mergeItemAged->vco = vco;
    menu->addChild(mergeItemAged);
}


Model *modelVCO = Model::create<VCO, VCOWidget>("Lindenberg Research", "VCO", "Woldemar VCO", OSCILLATOR_TAG);
