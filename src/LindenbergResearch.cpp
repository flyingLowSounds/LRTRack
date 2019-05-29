#include "LindenbergResearch.hpp"

Plugin *pluginInstance;


void init(Plugin *p) {
    pluginInstance = p;


    p->addModel(modelType35);
    p->addModel(modelDiodeVCF);
    p->addModel(modelBlankPanel);
    p->addModel(modelBlankPanelWood);
    p->addModel(modelVCO);
    p->addModel(modelSimpleFilter);
    p->addModel(modelReShaper);

/*
    p->addModel(modelMS20Filter);
    p->addModel(modelAlmaFilter);

    p->addModel(modelWestcoast);
    p->addModel(modelQuickMix);

    p->addModel(modelTestDriver);
    p->addModel(modelEchoBox);




    //  p->addModel(modelSpeck);


    p->addModel(modelBlankPanelEmpty);
    p->addModel(modelBlankPanelSmall);
    */
}
