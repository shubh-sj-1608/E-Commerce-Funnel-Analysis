import pandas as pd
import numpy as np 
import matplotlib.pyplot as plt
import seaborn as sns
from datetime import datetime
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots

import plotly.io as pio
pio.renderers.default = 'notebook'

# Corporate styling
plt.style.use('seaborn-v0_8')
sns.set_palette('husl')
print("Everything is fine")

# Load Dataset 
df = pd.read_csv("Funnel_analysis_data.csv")
df


# Basic steps to perform

print("Top 5 data\n")
display(df.head())

df.tail() # last 5 data

# random data from dataset
df.sample()

# find the total columns name and number of columns

print(df.columns)

print(f"\nTotal Number of Columns {df.columns.nunique()}")

#type of dataset 

type(df)

#datatype of all columns
df.dtypes

# timestamp col  must be in  the date time type 

df['Timestamp'] = pd.to_datetime(df['Timestamp'])
print(df.dtypes)

# information abt all the columns

df.info()

#imp info abt dataset's numeric columns
df.describe()

# imp info abt all the columns
df.describe(include = "str")

# shape of dataset
df.shape

pd.set_option("display.max_rows", None)
df

# Data preprocessing  and Cleaning

# Check for the null and duplicate values 
print("\n----Finding Null Values----\n")
null_values = df.isnull().sum()
print(null_values)

print("\n----Finding Duplicate Values----\n")
duplicate_values = df.duplicated().sum()
print(f"Total numbers of duplicatevalues in this dataset is{duplicate_values}")

print("\n----Total Uniques Values----")
unique_data = df.nunique()
print(unique_data)

# now time for fixing the data and extracting  time base data
# here we create multiple columns for deep analysis

df['Date'] = df['Timestamp'].dt.date
df['DayOfWeek'] = df['Timestamp'].dt.day_name()
df['Hour'] = df['Timestamp'].dt.hour
df['WeekNumber'] = df['Timestamp'].dt.isocalendar().week

df['Event_sequence'] = df.groupby('Session_ID').cumcount() +1

df.head()

# Basic steps

print(f'Total User ID {df['User_ID'].nunique()}')
print(f'\nTotal Session ID {df['Session_ID'].nunique()}')
print(f'\nDate range {df['Date'].min()} to {df['Date'].max()}')

#Funnel Stage Definition And Session-Level Aggregation
funnel_stages = ['Browse', 'Add to Cart', 'Checkout', 'Purchase']
# Create session-Level summary
session_summary = df.groupby('Session_ID').agg({
     'User_ID': 'first',
     'Timestamp': ['min', 'max'],
     'Event': lambda x: list(x),
     'Device': 'first',
     'Region': 'first',
     'Channel': 'first',
     'Product_Category': 'first',
     'Revenue': 'max',
     'Bounce_Flag':'first'
}).reset_index()

#Flatten column names
session_summary.columns = ['Session_ID', 'User_ID', 'Session_Start','Session_End', 'Event_Sequence', 'Device', 'Region',
                           'Channel','Product_Category', 'Revenue','Bounce_Flag']
#calculate session duration in minutes 
session_summary['Session_Duration_Min'] = (
    session_summary['Session_End'] - session_summary['Session_Start']
).dt.total_seconds() / 60

#Identify max funnel stage reached for each session
def get_max_funnel_stage(events):
    stage_values = {stage: i for i, stage in enumerate(funnel_stages)}
    max_stage_index = -1
    for event in events:
        if event in stage_values and stage_values[event] > max_stage_index: 
            max_stage_index = stage_values[event]
    return funnel_stages[max_stage_index] if max_stage_index != -1 else 'Browse'

session_summary['Max_Funnel_Stage'] = session_summary['Event_Sequence'].apply(get_max_funnel_stage)

print("Session Summary Created:")
display(session_summary.head())

#  calculate  overall  funnel analysis
funnel_metrics = []
for i, stage in enumerate(funnel_stages):
    if i == 0:
        #for browse stage, count all sessions
        count = len(session_summary)
    else:
        #for other stages, count sessions that reached at  least this stage
        count = len(session_summary[session_summary['Max_Funnel_Stage'].isin(funnel_stages[i:])])
  
    funnel_metrics.append({
        'Stage': stage,
        'Sessions': count,
        'Stage_Order': i
    })  

funnel_df = pd.DataFrame(funnel_metrics) 

#Calculate conversion rates
funnel_df['Conversion_Rate'] = (funnel_df['Sessions'] / funnel_df['Sessions'].iloc[0]*100).round(2)
funnel_df['Drop_Off_Rate'] = (1 - funnel_df['Sessions'] / funnel_df['Sessions'].shift(1)) * 100
funnel_df.loc[0, 'Drop_Off_Rate'] = 0
funnel_df['Drop_Off_Rate'] = funnel_df['Drop_Off_Rate' ].round(2)   

print(" Overall Funnel Analysis:")
display(funnel_df)

# Revenue analysis
revenue_stats = session_summary[session_summary['Max_Funnel_Stage'] == 'Purchase'].agg({
   'Revenue': ['sum', 'mean', 'count']
}).round(2)

print("\n Revenue Analysis:")
print(f"Total Revenue: ${revenue_stats.iloc[0, 0]:,.2f}")
print(f"Average Order Value: ${revenue_stats.iloc[1,0]:,.2f}")
print(f"Total Orders: {revenue_stats.iloc[2, 0]:,}")


# Funnel analysts by channel
channel_funnel = []

for channel in df['Channel'].unique():
    channel_sessions = session_summary[session_summary['Channel'] == channel]
    total_sessions = len(channel_sessions)

    if total_sessions > 0:
       channel_metrics = {'Channel': channel, 'Total_Sessions': total_sessions}

       for i, stage in enumerate(funnel_stages):
            if i == 0:
               count = total_sessions
            else:
                count = len(channel_sessions[channel_sessions['Max_Funnel_Stage'].isin(funnel_stages[i:])])

            channel_metrics[f'{stage}_Sessions'] = count
            channel_metrics[f'{stage}_Rate'] = (count / total_sessions * 100)

   # Revenue metrics
       purchase_sessions = channel_sessions[channel_sessions['Max_Funnel_Stage'] == 'Purchase']
       channel_metrics['Total_Revenue'] = purchase_sessions['Revenue'].sum()
       channel_metrics['AOV'] = purchase_sessions['Revenue'].mean() if len(purchase_sessions) > 0 else 0
       channel_metrics['Conversion_Rate'] = (len(purchase_sessions) / total_sessions *100 )

       channel_funnel.append(channel_metrics)

channel_df = pd.DataFrame(channel_funnel)
print("Channel Performance Analysis:")
display(channel_df.round(2))

# Visualize channel performance
fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))

# Conversion rates by channel
sns.barplot(data = channel_df, x='Channel', y='Conversion_Rate', ax=ax1)
ax1.set_title('Conversion Rate by Channel')
ax1.tick_params(axis='x', rotation=45)

# Total revenue by channel
sns.barplot(data = channel_df, x='Channel', y='Total_Revenue', ax=ax2)
ax2.set_title('Total Revenue by Channel')
ax2.tick_params(axis='x', rotation=45)

# AOV by channel
sns.barplot(data=channel_df, x='Channel', y='AOV', ax=ax3)
ax3.set_title('Average Order Value by Channel')

# AOV by channel
sns.barplot(data=channel_df, x='Channel', y='AOV', ax=ax3)
ax3.set_title('Average Order Value by Channel')
ax3.tick_params(axis='x', rotation=45)

# Session distribution by channel
channel_df['Session_Percentage'] = (channel_df['Total_Sessions'] / channel_df['Total_Sessions'].sum() * 100)
ax4.pie(channel_df['Session_Percentage'], labels=channel_df['Channel'], autopct='%1.1f%%')
ax4.set_title('Session Distribution by Channel')

plt.tight_layout()
plt.show()              

              
#Regional Analysis

#Regional Performance analysis
regional_analysis = session_summary.groupby('Region').agg({
    'Session_ID': 'count',
    'Revenue': 'sum',
    'Session_Duration_Min': 'mean'
}).rename(columns = {'Session_ID':'Total_Sessions', 'Revenue':'Total_Revenue'}) 

# Add conversion rates by region
regional_conversion = session_summary[session_summary['Max_Funnel_Stage'] == 'Purchase'].groupby('Region').size()
regional_analysis['Converted_Sessions'] = regional_conversion
regional_analysis['Conversion_Rate'] = (regional_analysis['Converted_Sessions'] / regional_analysis['Total_Sessions' ] * 100).round(2)
regional_analysis['AOV' ] = (regional_analysis['Total_Revenue'] / regional_analysis['Converted_Sessions']).round(2)

print("Regional Performance:")
display(regional_analysis)

# Regional funnel visualization
regional_funnel_data = []
for region in df['Region'].unique():
    region_sessions = session_summary[session_summary['Region' ] == region]
    for stage in funnel_stages:
        if stage == 'Browse':
            count = len(region_sessions)
        else:
            count = len(region_sessions[region_sessions['Max_Funnel_Stage' ].isin(funnel_stages[funnel_stages. index(stage) : ])])
        regional_funnel_data.append({'Region': region, 'Stage': stage, 'Sessions': count})

regional_funnel_df = pd.DataFrame(regional_funnel_data)

plt.figure(figsize=(12, 6))
sns.lineplot(data=regional_funnel_df, x='Stage', y='Sessions', hue='Region' , marker='o')
plt.title('Funnel Progression by Region')
plt.xticks(rotation=45)
plt.tight_layout()
plt. show()

# device and product category analysis
# Device performance

device_analysis = session_summary.groupby('Device').agg({
    'Session_ID': 'count',
    'Revenue': 'sum',
    'Session_Duration_Min': 'mean',
    'Max_Funnel_Stage': lambda x: (x == 'Purchase' ).sum()
}).rename(columns={'Session_ID': 'Total_Sessions', 'Max_Funnel_Stage': 'Purchases'})

device_analysis['Conversion_Rate'] = (device_analysis['Purchases'] / device_analysis['Total_Sessions'] * 100).round(2)
device_analysis['AOV'] = (device_analysis['Revenue'] / device_analysis['Purchases']).round(2)

print("Device Performance:")
display(device_analysis)

# Product category analysis
product_analysis = session_summary.groupby('Product_Category').agg({
    'Session_ID': 'count',
    'Revenue': 'sum',
    'Max_Funnel_Stage': lambda x: (x == 'Purchase').sum()
}).rename(columns={'Session_ID': 'Total_Sessions', 'Max_Funnel_Stage': 'Purchases' })

product_analysis['Conversion_Rate' ] = (product_analysis['Purchases' ] / product_analysis['Total_Sessions'] * 100).round(2)
product_analysis['AOV'] = (product_analysis['Revenue' ] / product_analysis['Purchases' ]).round(2)
product_analysis['Revenue_Per_Session' ] = (product_analysis['Revenue' ] / product_analysis['Total_Sessions']).round(2)

print(" Product Category Performance:")
display(product_analysis)

# Combined visualization
fig, (ax1, ax2)=plt.subplots(1, 2, figsize=(15,6))

# Device performance
sns.barplot(data=device_analysis.reset_index(), x='Device', y='Conversion_Rate', ax=ax1)
ax1.set_title('Conversion Rate by Device Type')

# Product category performance
sns.barplot(data=product_analysis.reset_index(), x='Product_Category', y='Conversion_Rate', ax=ax2)
ax2.set_title('Conversion Rate by Product Category')
ax2.tick_params(axis='x', rotation=45)

plt.tight_layout()
plt.show()

#time-based analysis

# Daily trends
daily_metrics = df.groupby('Date').agg({
    'Session_ID': 'nunique',
    'User_ID': 'nunique',
    'Revenue' : 'sum'
}).rename(columns={'Session_ID': 'Daily_Sessions', 'User_ID': 'Daily_Users'})

# Add conversion rates daily
# Modify the problematic line to extract date from datetime
daily_conversions = session_summary[session_summary['Max_Funnel_Stage'] == 'Purchase'].groupby(
    session_summary['Session_Start'].dt.date
).size()
daily_metrics['Daily_Conversions' ] = daily_conversions
daily_metrics['Daily_Conversion_Rate'] = (daily_metrics['Daily_Conversions' ] / daily_metrics['Daily_Sessions'] * 100).round(2)

print("Daily Performance Trends")
display(daily_metrics.tail(10))

# Hourly patterns
hourly_sessions = df.groupby('Hour').agg({
    'Session_ID': 'nunique',
    'Revenue' : 'sum'
}).rename(columns={'Session_ID': 'Hourly_Sessions'})

hourly_conversions = session_summary[session_summary['Max_Funnel_Stage' ] == 'Purchase']
hourly_conversions ['Hour' ] = hourly_conversions['Session_Start'].dt.hour
hourly_conversion_counts = hourly_conversions.groupby('Hour').size()
hourly_sessions ['Hourly_Conversions' ] = hourly_conversion_counts
hourly_sessions['Hourly_Conversion_Rate' ] = (hourly_sessions['Hourly_Conversions' ] / hourly_sessions['Hourly_Sessions'] * 100). round(2)

# Visualization
fig, ((ax1, ax2),(ax3,ax4))=plt.subplots(2, 2, figsize=(15, 10))

# Daily sessions
ax1.plot(daily_metrics. index, daily_metrics['Daily_Sessions'])
ax1.set_title('Daily Sessions Over Time')
ax1.tick_params(axis='x', rotation=45)

# Daily conversion rate
ax2.plot(daily_metrics. index, daily_metrics['Daily_Conversion_Rate' ])
ax2.set_title('Daily Conversion Rate')
ax2.tick_params(axis='x', rotation=45)

# Hourly session pattern
ax3.bar(hourly_sessions. index, hourly_sessions['Hourly_Sessions' ])
ax3.set_title('Sessions by Hour of Day')
ax3.set_xlabel('Hour of Day')

# Hourly conversion rate
ax4.bar(hourly_sessions. index, hourly_sessions['Hourly_Conversion_Rate' ])
ax4.set_title('Conversion Rate by Hour of Day')
ax4.set_xlabel('Hour of Day')

plt.tight_layout()
plt. show( )

# Calculate advanced metrics
print(" KEY PERFORMANCE INDICATORS (KPIs)")
print("="*50)

# OveraLL KPIs
total_sessions = len(session_summary)
total_revenue = session_summary['Revenue' ].sum()
total_orders = len(session_summary[session_summary['Max_Funnel_Stage' ] == 'Purchase' ])
overall_conversion_rate = (total_orders / total_sessions * 100)

print(f" Overall Conversion Rate: {overall_conversion_rate :.2f}%")
print(f" Total Revenue: ${total_revenue:,.2f}")
print(f" Average Order Value: ${(total_revenue/total_orders) :.2f}")
print(f" Total Sessions: {total_sessions:,}")
print(f" Total Orders: {total_orders:,}")

# Funnel efficiency metrics
browse_to_cart = (funnel_df.iloc[1]['Sessions'] / funnel_df.iloc[0] ['Sessions' ] * 100)
cart_to_checkout = (funnel_df.iloc[2]['Sessions'] / funnel_df.iloc[1]['Sessions' ] * 100)
checkout_to_purchase = (funnel_df.iloc[3]['Sessions' ] / funnel_df.iloc[2] ['Sessions' ] * 100)


print(f"\nStage-to-Stage Conversion Rates:")
print(f" Browse > Add to Cart: {browse_to_cart :.2f}%")
print(f"Add to Cart > Checkout: {cart_to_checkout :.2f}%")
print(f"Checkout > Purchase: {checkout_to_purchase :.2f}%")

# Revenue per session at each stage
revenue_per_browse = total_revenue / funnel_df.iloc[0]['Sessions']
revenue_per_cart = total_revenue / funnel_df.iloc[1]['Sessions' ]
revenue_per_checkout = total_revenue / funnel_df.iloc[2]['Sessions']

print(f"\n Revenue per Session by Stage:")
print(f"Per Browse Session: ${revenue_per_browse :.2f}")
print(f"Per Cart Session: ${revenue_per_cart :.2f}")
print(f"Per Checkout Session: ${revenue_per_checkout :.2f}")

# Bounce rate analysis
bounce_sessions = session_summary[session_summary['Bounce_Flag' ] == 'Yes']
bounce_rate = (len(bounce_sessions) / total_sessions * 100)
print(f"\nBounce Rate: {bounce_rate :.2f}%")

# Session duration analysis
avg_session_duration = session_summary['Session_Duration_Min' ].mean()
print(f"Average Session Duration: {avg_session_duration :.2f} minutes")


# Strategic recommendation

# Generate actionable insights
print(" STRATEGIC RECOMMENDATIONS")
print("="*50)

# Identify biggest drop-off points
max_dropoff_stage = funnel_df.loc[funnel_df['Drop_Off_Rate'].idxmax()]
print(f" BIGGEST DROP-OFF: {max_dropoff_stage['Stage' ]} stage with {max_dropoff_stage['Drop_Off_Rate']}% drop-off")

# Best performing channel
best_channel = channel_df.loc[channel_df['Conversion_Rate'].idxmax()]
print(f" BEST PERFORMING CHANNEL: {best_channel['Channel']} with {best_channel['Conversion_Rate' ]}% conversion")

# Best performing region
best_region = regional_analysis. loc[regional_analysis['Conversion_Rate'].idxmax()]
print(f" BEST PERFORMING REGION: {best_region.name} with {best_region['Conversion_Rate' ]}% conversion")

# Best performing product category
best_category = product_analysis.loc[product_analysis['Conversion_Rate'].idxmax()]
print(f" BEST PERFORMING CATEGORY: {best_category.name} with {best_category['Conversion_Rate' ]}% conversion")

print(f"\n ? RECOMMENDED ACTIONS:")
print(f"1. Address {max_dropoff_stage['Stage']} stage drop-off through UX improvements")
print(f"2. Allocate more budget to {best_channel['Channel']} channel")
print(f"3. Replicate {best_region.name} region strategies in underperforming regions")
print(f"4. Promote {best_category.name} category to improve overall conversion")
print(f"5. Focus on cart abandonment recovery for {funnel_df.iloc[2]['Drop_Off_Rate']}% of users")

# Calculate potential revenue opportunities
cart_abandonment_opportunity = funnel_df. iloc[2] ['Sessions' ] *product_analysis[ 'AOV' ]. mean()
print(f"\nRevenue Opportunity:")
print(f" Cart abandonmant recovery :${cart_abandonment_opportunity:,.2f} potential revenue")

#Export Result
# Create comprehensive report
report_data = {
    'Overall_Funnel': funnel_df,
    'Channel_Performance': channel_df,
    'Regional_Analysis': regional_analysis,
    'Device_Performance': device_analysis,
    'Product_Performance': product_analysis,
    'Daily_Trends': daily_metrics,
    'Hourly_Patterns': hourly_sessions
}
# Export to Excel for corporate reporting
with pd.ExcelWriter('funnel_analysis_report.xlsx') as writer:
    for sheet_name, data in report_data.items():
        data.to_excel(writer, sheet_name=sheet_name, index=False)

print("Analysis complete! Report exported to 'funnel_analysis_report.xlsx'")

# Save key visualizations
plt.figure(figsize=(10, 6))
sns.barplot(data=funnel_df, x='Stage', y='Conversion_Rate')
plt.title('Overall Funnel Conversion Rates')
plt.xticks(rotation=45)
plt.tight_layout ()
plt.savefig('funnel_conversion_rates.png', dpi=300, bbox_inches='tight')

print("Key visualizations saved!")
