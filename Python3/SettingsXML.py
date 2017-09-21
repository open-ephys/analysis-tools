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



def FindRecProcs(Ch, Proc, RecChs):
    ChNo = Ch['number']
    Rec = Proc['CHANNEL'][ChNo]['SELECTIONSTATE']['record']
    
    if Rec == '1':
        if Proc['NodeId'] not in RecChs: RecChs[Proc['NodeId']] = {}
        RecChs[Proc['NodeId']][ChNo] = Ch
    
    return(RecChs)


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
        
        return(Dict)
    else:
        if El.items(): return(dict(El.items()))
        else: return(El.text)


def XML2Dict(File):
    Tree = ElementTree.parse(File); Root = Tree.getroot()
    Info = Root2Dict(Root)
    
    return(Info)


def GetRecChs(File):
    Info = XML2Dict(File)
    RecChs = {}; ProcNames = {}
    
    for P, Proc in Info['SIGNALCHAIN']['PROCESSOR'].items():
        if 'isSource' in Proc: 
            if Proc['isSource'] == '1': SourceProc = P[:]
        else:
            if Proc['name'].split('/')[0] == 'Sources': SourceProc = P[:]
        
        if 'CHANNEL_INFO' in Proc and Proc['CHANNEL_INFO']:
            for Ch in Proc['CHANNEL_INFO']['CHANNEL'].values():
                RecChs = FindRecProcs(Ch, Proc, RecChs)
            
        elif 'CHANNEL' in Proc:
            for Ch in Proc['CHANNEL'].values():
                RecChs = FindRecProcs(Ch, Proc, RecChs)
            
        else: continue
        
        if 'pluginName' in Proc:
            ProcNames[Proc['NodeId']] = Proc['pluginName']
        else:
            ProcNames[Proc['NodeId']] = Proc['name']
    
    if Info['SIGNALCHAIN']['PROCESSOR'][SourceProc]['CHANNEL_INFO']:
        SourceProc = Info['SIGNALCHAIN']['PROCESSOR'][SourceProc]['CHANNEL_INFO']['CHANNEL']
    else:
        SourceProc = Info['SIGNALCHAIN']['PROCESSOR'][SourceProc]['CHANNEL']
    
    for P, Proc in RecChs.items():
        for C, Ch in Proc.items():
            if 'gain' not in Ch:
                RecChs[P][C].update([c for c in SourceProc.values() if c['number'] == C][0])
    
    return(RecChs, ProcNames)

