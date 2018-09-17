#pragma once

#include "rack.hpp"
#include "asset.hpp"
#include "widgets.hpp"

#define LCD_FONT_DIG7 "res/digital-7.ttf"
#define LCD_FONTSIZE 11
#define LCD_LETTER_SPACING 0

#define LED_DEFAULT_COLOR nvgRGBAf(0.1, 0.3, 0.9, 0.99)
#define LED_RED_COLOR nvgRGBAf(0.9, 0.1, 0.1, 0.99)
#define LED_GREEN_COLOR nvgRGBAf(0.1, 0.9, 0.3, 0.99)

/* show values of all knobs */
#define DEBUG_VALUES false

using namespace rack;

extern Plugin *plugin;

namespace lrt {

typedef std::shared_ptr<rack::Font> TrueType;


/**
 * @brief Emulation of a LCD monochrome display
 */
struct LRLCDWidget : Label {

    enum LCDType {
        NUMERIC,
        TEXT,
        LIST
    };

    TrueType ttfLCDDig7;
    float fontsize;

    LCDType type;

    NVGcolor fg;
    NVGcolor bg;

    bool active = true;
    float value = 0.0;
    unsigned char length = 0;
    std::string format;
    std::vector<std::string> items;

    std::string s1;
    std::string s2;

    /**
     * @brief Constructor
     */
    LRLCDWidget(NVGcolor fg, unsigned char length, std::string format, LCDType type, float fontsize = LCD_FONTSIZE);

    /**
     * @brief Draw LCD display
     * @param vg
     */
    void draw(NVGcontext *vg) override;


    inline void addItem(std::string name) {
        items.push_back(name);
    }
};


/**
 * @brief Indicator for control voltages on knobs
 */
struct LRCVIndicator : TransparentWidget {
    static constexpr float OVERFLOW_THRESHOLD = 0.01f;

    /** flag to control drawing */
    bool active = false;

    /** color of indicator */
    NVGcolor normalColor = nvgRGBA(0x00, 0x00, 0x00, 0xBB);
    NVGcolor overflowColor = nvgRGBA(0xBB, 0x00, 0x00, 0xBB);

    /** radius from middle */
    float distance;

    /** normalized control voltage. must between [0..1] */
    float cv = 0.f;

    /** indicator distances */
    float d1 = 4.f;
    float d2 = 0.1f;

    /** draw angle */
    float angle;
    float angle2;

    /** middle of parent */
    Vec middle;


    /**
     * @brief Init indicator
     * @param distance Radius viewed from the middle
     * @param angle Angle of active knob area
     */
    LRCVIndicator(float distance, float angle);


    /**
     * @brief Manipulate the indicator symbol
     * @param d1 Height of the triangle
     * @param d2 Half of the base width
     */
    void setDistances(float d1, float d2);

    /**
     * @brief Draw routine for cv indicator
     * @param vg
     */
    void draw(NVGcontext *vg) override;
};


/**
 * @brief Standard LR Shadow
 */
struct LRShadow : TransparentWidget {
private:
    Rect box;
    float size = 0.65;
    float strength = 1.f;

    /** shadow shift */
    Vec shadowPos = Vec(3, 5);
public:


    LRShadow();


    /**
     * @brief Set the new offset of the shadow gradient
     * @param x
     * @param y
     */
    void setShadowPosition(float x, float y) {
        shadowPos = Vec(x, y);
    }


    void setBox(const Rect &box);
    void setSize(float size);
    void setStrength(float strength);

    /**
    * @brief Draw shadow for circular knobs
    * @param vg NVGcontext
    * @param strength Alpha value of outside gradient
    * @param size Outer size
    * @param shift XY Offset shift from middle
    */
    void drawShadow(NVGcontext *vg, float strength, float size);

    void draw(NVGcontext *vg) override;
};


/**
 * @brief The base of all knobs used in LR panels, includes a indicator
 */
struct LRKnob : SVGKnob {
private:
    static constexpr float ANGLE = 0.83f;

    /** setup indicator with default values */
    LRCVIndicator *indicator;

    bool debug = DEBUG_VALUES;
    TrueType font;

    /** snap mode */
    bool snap = false;
    /** position to snap */
    float snapAt = 0.0f;
    /** snap sensitivity */
    float snapSens = 0.1;

protected:
    /** shader */
    LRShadow *shader;


public:

    LRKnob();


    /**
     * @brief Set the value of the indicator
     * @param value
     */
    void setIndicatorValue(float value) {
        indicator->cv = value;
        dirty = true;
    }


    /**
     * @brief Switch indicator on/off
     * @param active
     */
    void setIndicatorActive(bool active) {
        indicator->active = active;
        dirty = true;
    }


    /**
     * @brief Get indicator state
     * @return
     */
    bool isIndicatorActive() {
        return indicator->active;
    }


    /**
     * @brief Setup distance of indicator from middle
     * @param distance
     */
    void setIndicatorDistance(float distance) {
        indicator->distance = distance;
        dirty = true;
    }


    /**
     * @brief Setup distance of indicator from middle
     * @param distance
     */
    void setIndicatorShape(float d1, float d2) {
        indicator->setDistances(d1, d2);
        dirty = true;
    }


    /**
     * @brief Set new colors for the indicator (default red overflow)
     * @param normal Normal indicator color
     * @param overflow Optional overflow color
     */
    void setIndicatorColors(NVGcolor normal, NVGcolor overflow = nvgRGBA(0xBB, 0x00, 0x00, 0xBB)) {
        indicator->normalColor = normal;
        indicator->overflowColor = overflow;
    }


    /**
     * @brief Hook into setSVG() method to setup box dimensions correct for indicator
     * @param svg
     */
    void setSVG(std::shared_ptr<SVG> svg);


    /**
     * @brief Creates a new instance of a LRKnob child
     * @tparam TParamWidget Subclass of LRKnob
     * @param pos Position
     * @param module Module pointer
     * @param paramId Parameter ID
     * @param minValue Min
     * @param maxValue Max
     * @param defaultValue Default
     * @return Pointer to new subclass of LRKnob
     */
    template<class TParamWidget>
    static TParamWidget *create(Vec pos, Module *module, int paramId, float minValue, float maxValue, float defaultValue) {
        auto *param = new TParamWidget();
        param->box.pos = pos;
        param->module = module;
        param->paramId = paramId;
        param->setLimits(minValue, maxValue);
        param->setDefaultValue(defaultValue);
        return param;
    }


    /**
     * @brief Draw knob
     * @param vg
     */
    void draw(NVGcontext *vg) override;

    /**
     * @brief Setup knob snapping
     * @param position
     * @param sensitivity
     */
    void setSnap(float position, float sensitivity);

    /**
     * @brief Remove knob snaping
     */
    void unsetSnap();


    /**
     * @brief Snapping mode for knobs
     * @param e
     */
    void onChange(EventChange &e) override;
};


/**
 * @brief Quantize position to odd numbers to simulate a toggle switch
 */
struct LRToggleKnob : LRKnob {
    LRToggleKnob(float length = 0.5f) {
        //TODO: parametrize start and end angle
        minAngle = -0.666666f * (float) M_PI;
        maxAngle = length * (float) M_PI;

        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/ToggleKnob.svg")));
        shader->setShadowPosition(3, 4);

        shader->setStrength(1.2f);
        shader->setSize(0.7f);

        speed = 2.f;
    }


    void onChange(EventChange &e) override {
        value = round(value);
        SVGKnob::onChange(e);
    }
};


/**
 * @brief Quantize position to odd numbers to simulate a toggle switch
 */
struct LRAlternateToggleKnobLight : LRKnob {
    LRAlternateToggleKnobLight(float length = 0.5f) {
        //TODO: parametrize start and end angle
        minAngle = -0.666666f * (float) M_PI;
        maxAngle = length * (float) M_PI;

        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateToggleKnobLight.svg")));
        setIndicatorDistance(11);
        setIndicatorShape(4.3, 0.11);


        shader->setShadowPosition(2, 3);

        shader->setStrength(0.5f);
        shader->setSize(0.6f);

        speed = 2.f;
    }


    void onChange(EventChange &e) override {
        value = round(value);
        SVGKnob::onChange(e);
    }
};

/**
 * @brief Quantize position to odd numbers to simulate a toggle switch
 */
struct LRMiddleIncremental : LRKnob {
    LRMiddleIncremental(float length = 0.5f) {
        minAngle = -length * (float) M_PI;
        maxAngle = length * (float) M_PI;

        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateMiddleKnob.svg")));
        shader->setShadowPosition(3, 4);

        shader->setStrength(1.2f);
        shader->setSize(0.7f);

        speed = 3.f;
    }


    void onChange(EventChange &e) override {

        value = lround(value);

        //value = round(value);
        SVGKnob::onChange(e);
    }
};


/**
 * @brief LR Big Knob
 */
struct LRBigKnob : LRKnob {
    LRBigKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/BigKnob.svg")));
        setIndicatorDistance(15);
        setIndicatorShape(4.8, 0.12);
        shader->setShadowPosition(5, 6);
    }
};


/**
 * @brief LR Middle Knob
 */
struct LRMiddleKnob : LRKnob {
    LRMiddleKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/MiddleKnob.svg")));
        setIndicatorDistance(13);
        setIndicatorShape(5, 0.13);
        shader->setShadowPosition(4, 4);
    }
};


/**
 * @brief LR Small Knob
 */
struct LRSmallKnob : LRKnob {
    LRSmallKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/SmallKnob.svg")));
        shader->setShadowPosition(3, 3);
        setSnap(0.0f, 0.02f);


        speed = 0.9f;
    }
};


/**
 * @brief LR Alternate Small Knob
 */
struct LRAlternateSmallKnob : LRKnob {
    LRAlternateSmallKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateSmallKnob.svg")));
        shader->setShadowPosition(3, 3);
        setSnap(0.0f, 0.02f);


        speed = 0.9f;
    }
};


/**
 * @brief LR Middle Knob
 */
struct LRAlternateMiddleKnob : LRKnob {
    LRAlternateMiddleKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateMiddleKnob.svg")));
        setIndicatorDistance(11);
        setIndicatorShape(5.0, 0.14);
        shader->setShadowPosition(4, 5);

        setSnap(0.0f, 0.12f);
    }
};


/**
 * @brief LR Big Knob
 */
struct LRAlternateBigKnob : LRKnob {
    LRAlternateBigKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateBigKnob.svg")));
        setIndicatorDistance(15);
        setIndicatorShape(4.8, 0.12);
        shader->setShadowPosition(5, 6);
    }
};


/**
 * @brief LR Big Knob
 */
struct LRAlternateBigLight : LRKnob {
    LRAlternateBigLight() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateBigLight.svg")));
        setIndicatorDistance(17);
        setIndicatorShape(4.1, 0.08);


        shader->setShadowPosition(4, 5);

        shader->setStrength(0.5f);
        shader->setSize(0.6f);

    }
};


/**
 * @brief LR Big Knob
 */
struct LRAlternateMiddleLight : LRKnob {
    LRAlternateMiddleLight() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateMiddleLight.svg")));
        setIndicatorDistance(11);
        setIndicatorShape(4.3, 0.11);


        shader->setShadowPosition(2, 3);

        shader->setStrength(0.5f);
        shader->setSize(0.6f);

    }
};


/**
 * @brief LR Small Knob
 */
struct LRAlternateSmallLight : LRKnob {
    LRAlternateSmallLight() {
        setSVG(SVG::load(assetPlugin(plugin, "res/knobs/AlternateSmallLight.svg")));
        shader->setShadowPosition(3, 3);
        setSnap(0.0f, 0.02f);

        shader->setShadowPosition(2, 3);

        shader->setStrength(0.5f);
        shader->setSize(0.7f);

        speed = 0.9f;
    }
};


/**
 * @brief Alternative IO Port
 */
struct LRIOPort : SVGPort {
private:
    LRShadow *shader;

public:
    LRIOPort() {
        background->svg = SVG::load(assetPlugin(plugin, "res/elements/IOPortB.svg"));
        background->wrap();
        box.size = background->box.size;

        shader = new LRShadow();
        //  addChild(shadow);

        /** inherit dimensions */
        shader->setBox(box);
        shader->setSize(0.50);
        shader->setShadowPosition(3, 2);
    }


    /**
     * @brief Hook into draw method
     * @param vg
     */
    void draw(NVGcontext *vg) override {
        shader->draw(vg);
        SVGPort::draw(vg);
    }
};


/**
 * @brief Alternative IO Port
 */
struct LRIOPortBLight : SVGPort {
private:
    LRShadow *shader;

public:
    LRIOPortBLight() {
        background->svg = SVG::load(assetPlugin(plugin, "res/elements/IOPortBLight.svg"));
        background->wrap();
        box.size = background->box.size;

        shader = new LRShadow();
        //  addChild(shadow);

        /** inherit dimensions */
        shader->setBox(box);
        shader->setSize(0.55);
        shader->setStrength(0.3);
        shader->setShadowPosition(1, 2);
    }


    /**
     * @brief Hook into draw method
     * @param vg
     */
    void draw(NVGcontext *vg) override {
        shader->draw(vg);
        SVGPort::draw(vg);
    }
};


/**
 * @brief Alternative IO Port
 */
struct LRIOPortC : SVGPort {
private:
    LRShadow *shader;

public:
    LRIOPortC() {
        background->svg = SVG::load(assetPlugin(plugin, "res/elements/IOPortC.svg"));
        background->wrap();
        box.size = background->box.size;

        shader = new LRShadow();
        // addChild(shader);

        /** inherit dimensions */
        shader->setBox(box);
        shader->setSize(0.50);
        shader->setStrength(0.1);
        shader->setShadowPosition(3, 4);
    }


    /**
     * @brief Hook into draw method
     * @param vg
     */
    void draw(NVGcontext *vg) override {
        shader->draw(vg);
        SVGPort::draw(vg);
    }
};


/**
 * @brief Alternative IO Port
 */
struct LRIOPortCLight : SVGPort {
private:
    LRShadow *shader;

public:
    LRIOPortCLight() {
        background->svg = SVG::load(assetPlugin(plugin, "res/elements/IOPortCLight.svg"));
        background->wrap();
        box.size = background->box.size;

        shader = new LRShadow();
        // addChild(shader);

        /** inherit dimensions */
        shader->setBox(box);
        shader->setSize(0.55);
        shader->setStrength(0.3);
        shader->setShadowPosition(1, 2);
    }


    /**
     * @brief Hook into draw method
     * @param vg
     */
    void draw(NVGcontext *vg) override {
        shader->draw(vg);
        SVGPort::draw(vg);
    }
};


/**
 * @brief Alternative screw head A
 */
struct ScrewDarkA : SVGScrew {
    ScrewDarkA() {
        sw->svg = SVG::load(assetPlugin(plugin, "res/elements/ScrewDark.svg"));
        sw->wrap();
        box.size = sw->box.size;
    }
};


/**
 * @brief Alternative screw head A
 */
struct ScrewLight : SVGScrew {
    ScrewLight() {
        sw->svg = SVG::load(assetPlugin(plugin, "res/elements/ScrewLight.svg"));
        sw->wrap();
        box.size = sw->box.size;
    }
};


/**
 * @brief Alternative screw head A
 */
struct AlternateScrewLight : SVGScrew {
    AlternateScrewLight() {
        sw->svg = SVG::load(assetPlugin(plugin, "res/elements/AlternateScrewLight.svg"));
        sw->wrap();
        box.size = sw->box.size;
    }
};


/**
 * @brief Alternative screw head A
 */
struct ScrewDarkB : SVGScrew {
    ScrewDarkB() {
        sw->svg = SVG::load(assetPlugin(plugin, "res/elements/ScrewDarkB.svg"));
        sw->wrap();
        box.size = sw->box.size;
    }
};


/**
 * @brief Custom switch based on original Rack files
 */
struct LRSwitch : SVGSwitch, ToggleSwitch {
    LRSwitch() {
        addFrame(SVG::load(assetPlugin(plugin, "res/elements/Switch0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/elements/Switch1.svg")));
    }
};


/**
 * @brief Standard LED
 */
struct LRLight : ModuleLightWidget {
    LRLight();

    void draw(NVGcontext *vg) override;
};


/**
 * @brief Standard LR module Panel
 */
struct LRPanel : SVGPanel {
private:
    /** margin of gradient box */
    static constexpr float MARGIN = 10;

    NVGcolor bgColor = nvgRGBAf(0.0859375f, 0.0859375f, 0.0859375f, 1.f);
    bool colorOnly = false;

    /** gradient colors */
    NVGcolor inner = nvgRGBAf(1.5f * .369f, 1.5f * 0.357f, 1.5f * 0.3333f, 0.33f);
    NVGcolor outer = nvgRGBAf(0.0f, 0.0f, 0.0f, 0.1f);;


    /** gradient offset */
    Vec offset = Vec(30, -50);

    /* if gradient should be limited */
    Vec limit;

public:
    LRPanel();


    LRPanel(float x, float y, Vec limit = Vec(0.f, 0.f)) {
        offset.x = x;
        offset.y = y;

        LRPanel::limit = limit;
    }


    void setColorOnly() {
        colorOnly = true;
    }


    void setInner(const NVGcolor &inner);
    void setOuter(const NVGcolor &outer);
    const NVGcolor &getInner() const;
    const NVGcolor &getOuter() const;

    void draw(NVGcontext *vg) override;
};


/**
 * @brief Standard LRT ModuleWidget definition
 */
struct LRModuleWidget : ModuleWidget {

    LRPanel *panel;


    /**
     * @brief
     * @param module
     */
    LRModuleWidget(Module *module) : ModuleWidget(module) {
    }
};


/**
 * @brief Passive rotating SVG image
 */
struct SVGRotator : FramebufferWidget {
    TransformWidget *tw;
    SVGWidget *sw;

    /** angle to rotate per step */
    float angle = 0;
    float inc;
    float scale;


    SVGRotator();


    /**
     * @brief Factory method
     * @param pos Position
     * @param svg Pointer to SVG image
     * @param angle Increment angle per step
     */
    SVGRotator static *create(Vec pos, std::shared_ptr<SVG> svg, float inc, float scale = 1.0f) {
        SVGRotator *rotator = FramebufferWidget::create<SVGRotator>(pos);

        rotator->setSVG(svg);
        rotator->inc = inc;
        rotator->scale = scale;

        return rotator;
    }


    void setSVG(std::shared_ptr<SVG> svg);
    void step() override;
};


struct FontIconWidget : FramebufferWidget {
    TrueType iconFont;
    float fontSize;
    NVGcolor color;

    explicit FontIconWidget(float fontSize = 12.f, NVGcolor color = nvgRGBAf(1.f, 1.f, 1.f, 1.f));

    void draw(NVGcontext *vg) override;
};


/**
 * Utility widget for resize action on modules
 */
struct ModuleResizeWidget : Widget {

    float minWidth;
    bool right = false;
    float dragX;
    float dragY;
    Rect originalBox;


    ModuleResizeWidget(float _minWidth) {
        box.size = Vec(RACK_GRID_WIDTH * 1, RACK_GRID_HEIGHT);
        minWidth = _minWidth;
    }


    void onMouseDown(EventMouseDown &e) override {
        if (e.button == 0) {
            e.consumed = true;
            e.target = this;
        }
    }


    void onDragStart(EventDragStart &e) override {
        dragX = gRackWidget->lastMousePos.x;
        dragY = gRackWidget->lastMousePos.y;
        ModuleWidget *m = getAncestorOfType<ModuleWidget>();
        originalBox = m->box;
    }


    void onDragMove(EventDragMove &e) override {
        ModuleWidget *m = getAncestorOfType<ModuleWidget>();

        float newDragX = gRackWidget->lastMousePos.x;
        float deltaX = newDragX - dragX;
        float newDragY = gRackWidget->lastMousePos.y;
        float deltaY = newDragY - dragY;

        Rect newBox = originalBox;

        // resize width
        if (right) {
            newBox.size.x += deltaX;
            newBox.size.x = fmaxf(newBox.size.x, minWidth);
            newBox.size.x = roundf(newBox.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
        } else {
            newBox.size.x -= deltaX;
            newBox.size.x = fmaxf(newBox.size.x, minWidth);
            newBox.size.x = roundf(newBox.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
            newBox.pos.x = originalBox.pos.x + originalBox.size.x - newBox.size.x;
        }

        // resize height
        newBox.size.y += deltaY;
        newBox.size.y = fmaxf(newBox.size.y, RACK_GRID_HEIGHT);
        newBox.size.y = roundf(newBox.size.y / RACK_GRID_HEIGHT) * RACK_GRID_HEIGHT;

        gRackWidget->requestModuleBox(m, newBox);
    }

};
}