# -*- coding: utf-8 -*-
"""
Created on 20170704 21:15:19

@author: Thawann Malfatti

Loads info from the settings.xml file.

Examples:
    File = '/Path/To/Experiment/settings.xml
    
    # To get all info the xml file can provide:
    AllInfo = SettingsXML.XML2Dict(File)
    
    # AllInfo will be a dictionary following the same structure of the XML file.
    
    # To get info only about channels recorded:
    RecChs = SettingsXML.GetRecChs(File)[0]
    
    # To get also the processor names:
    RecChs, PluginNames = SettingsXML.GetRecChs(File)
    
    # RecChs will be a dictionary:
    # 
    # RecChs
    #     ProcessorNodeId
    #         ChIndex
    #             'name'
    #             'number'
    #             'gain'
    #         'PluginName'

"""

from xml.etree import ElementTree


def Root2Dict(El):
    Dict = {}
    if El.getchildren(): 
        for SubEl in El:
            if SubEl.keys():
                if SubEl.get('name'): 
                    if SubEl.tag not in Dict: Dict[SubEl.tag] = {}
                    Dict[SubEl.tag][SubEl.get('name')] = Root2Dict(SubEl)
                
                    Dict[SubEl.tag][SubEl.get('name')].update(
                        {K: SubEl.get(K) for K in SubEl.keys() if K is not 'name'}
                    )
                
                else: 
                    Dict[SubEl.tag] = Root2Dict(SubEl)
                    Dict[SubEl.tag].update(
                        {K: SubEl.get(K) for K in SubEl.keys() if K is not 'name'}
                    )
                    
                
            else: Dict[SubEl.tag] = Root2Dict(SubEl)
    else:
        if El.items(): Dict[El.tag] = dict(El.items())
        else: Dict[El.tag] = El.text
    
    
    return(Dict)


def XML2Dict(File):
    Tree = ElementTree.parse(File); Root = Tree.getroot()
    Info = Root2Dict(Root)
    
    return(Info)


def GetRecChs(File):
    Info = XML2Dict(File)
    RecChs = {}; ProcNames = {}
    
    for P, Proc in Info['SIGNALCHAIN']['PROCESSOR'].items():
        if 'CHANNEL_INFO' not in Proc: continue
        
        for C, Ch in Proc['CHANNEL_INFO']['CHANNEL'].items():
            ChNo = Ch['CHANNEL']['number']
            Rec = Proc['CHANNEL'][ChNo]['SELECTIONSTATE']['SELECTIONSTATE']['record']
            if Rec:
                if Proc['NodeId'] not in RecChs: RecChs[Proc['NodeId']] = {}
                RecChs[Proc['NodeId']][ChNo] = Ch['CHANNEL']
        
        ProcNames[Proc['NodeId']] = Proc['pluginName']
    
    return(RecChs, ProcNames)

