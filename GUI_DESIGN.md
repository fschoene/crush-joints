# CrushJoints - GUI Design Specification

This document outlines the complete GUI design for the CrushJoints plugin, matching the original CamelCrusher interface as specified in the research documentation.

## Overall Layout Structure

The GUI follows the exact layout shown in the research documentation:

```
┌────────────────────────────────────────────────────────┐
│  PATCH SELECTOR   [Preset Name]        RANDOMIZE       │
│                                                          │
│  [ON]  DISTORTION                                        │
│          TUBE  [knob]    MECH  [knob]                      │
│                                                          │
│  [ON]  FILTER                                            │
│          CUTOFF  [knob]  RES  [knob]                       │
│                                                          │
│  [ON]  COMPRESSOR                                        │
│          AMOUNT  [knob]  PHAT MODE  [button]            │
│                                                          │
│  [ON]  MASTER                                            │
│          VOLUME  [knob]  MIX  [knob]                     │
└────────────────────────────────────────────────────────┘
```

## GUI Components

### 1. Patch Selector Section
- **Patch Selector**: Dropdown menu for preset selection
- **Preset Name Display**: Central display showing current preset name
- **Randomize Button**: Large button to randomize active module parameters

### 2. Distortion Module Section
- **Distortion Group**: Blue "ON" toggle switch (upper-left of section)
- **Tube Knob**: Parameter control for tube distortion amount (0-100%)
- **Mech Knob**: Parameter control for mechanical distortion amount (0-100%)

### 3. Filter Module Section  
- **Filter Group**: Blue "ON" toggle switch (upper-left of section)
- **Cutoff Knob**: Frequency control (20 Hz to 20,000 Hz)
- **Resonance Knob**: Resonance control (0-100%)

### 4. Compressor Module Section
- **Compressor Group**: Blue "ON" toggle switch (upper-left of section)
- **Amount Knob**: Compression intensity (0-100%)
- **Phat Mode Button**: Toggle for parallel compression + bass boost

### 5. Master Section
- **Master Group**: Blue "ON" toggle switch (upper-left of section)
- **Volume Knob**: Output level control (-51 to 0 dB)
- **Mix Knob**: Dry/wet blend control (0-100%)

## Required GUI Features

### Component Behaviors

#### Blue On/Off Toggle Switches
- **Location**: Upper-left of each module section
- **Behavior**: Illuminated when active, dimmed when bypassed
- **Function**: Enable/disable entire module processing

#### Knob Controls
- **Linear Mode**: Default drag vertically for adjustment
- **Circular Mode**: Drag like a hardware knob  
- **Right-Click Menu**: 
  - Linear/Circular mode selection
  - MIDI Learn assignment
  - Value display toggling
- **Value Readout**: Shows parameter values when knobs are adjusted
- **Presets**: Each module has active/inactive state

#### Patch Selector
- **Dropdown**: Lists all 12 presets
- **Current Name Display**: Shows active preset name in central area
- **Navigation**: Up/down arrows for preset selection

### Advanced Features

#### Randomize Button
- **Function**: Assigns new values to every parameter in active modules
- **Intelligence**: Doesn't create settings that make no sound
- **Location**: Top right corner

#### Output Level Meter
- **Location**: Bottom section of GUI
- **Display**: Visual indication of clipping (if signal exceeds safe levels)
- **Behavior**: Alerts user when output is potentially clipping

#### MIDI Learn
- **Support**: All knobs support MIDI learn via right-click menu
- **Integration**: Follows standard JUCE MIDI learn patterns

#### Value Readout
- **Display**: Shows current preset name or parameter values
- **Activation**: Appears when knob is adjusted
- **Location**: Central area of GUI

## Component Specification Details

### Module Layout and Spacing
- **Modules**: Clearly separated by groups with blue on/off indicators  
- **Knobs**: Horizontal arrangement within each module section
- **Labeling**: Clear parameter names for each control
- **Visual Hierarchy**: Most important controls (Tube, Cutoff, Amount, Volume) prominently positioned

### Color Scheme
- **Blue**: Primary on/off switches (matching original)
- **White/Gray**: Background and controls
- **Red**: Critical warnings (clipping indicator)
- **Green**: Normal operation indicators

### Responsiveness
- **Layout**: Automatically adjusts to window size changes
- **Component Scaling**: Controls resize appropriately
- **Text Display**: Readout responsive to content changes

## GUI Implementation Plan

### Stage 1: Basic Structure
- Layout components as specified in research document
- Implement all required GUI elements
- Connect components to parameter system

### Stage 2: Interactive Features
- Implement slider behavior (both linear and circular)
- Add right-click context menus
- Connect patch selector to preset system
- Implement randomize button functionality

### Stage 3: Advanced Features
- Add output level meter
- Implement MIDI learn support
- Add value readout functionality
- Finalize visual styling to match original

### Stage 4: Polish and Testing
- Validate all GUI interactions
- Test with actual audio processing
- Ensure all elements respond properly to parameter changes
- Implement final UI styling to match authenticated original color scheme

## Integration with Plugin Architecture

### Parameter Binding
- All GUI components directly bound to corresponding parameters in PluginProcessor
- Changes in GUI update parameters immediately
- Parameter updates reflected in GUI in real-time
- Concurrent editing support (A/B testing)

### State Management
- Module on/off status reflects in GUI toggle states
- Preset switching updates all visible parameters
- UI properly reflects active and inactive components

### Audio Playground
- Real-time audio processing feedback through level meters
- Interactive parameter adjustment during playback
- Visual feedback for active/passed modules

This implementation will create a clear, professional, and highly functional GUI that matches the authentic look and feel of the original CamelCrusher, while maintaining all functionality needed for the full signal chain to be controlled.

## Implementation Notes

The actual GUI files (PluginEditor.cpp, etc.) would be created using:
1. JUCE's component hierarchy 
2. Property-based component creation
3. Direct parameter binding
4. Theme-styling to mimic original appearance
5. Proper RTL/LTR language support
6. Accessibility and keyboard navigation

All these components would use standard JUCE patterns for performance and compatibility across different host applications.