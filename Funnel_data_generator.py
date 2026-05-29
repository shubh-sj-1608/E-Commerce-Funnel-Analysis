import pandas as pd
import random
from faker import Faker
from datetime import timedelta

fake = Faker()
data = []

stages = ['Browse', 'Add to Cart', 'Checkout', 'Purchase']
probabilities = { 
    'Browse': 1.0,
    'Add to Cart': 0.7,
    'Checkout': 0.5,
    'Purchase': 0.3
}

devices = ['Mobile', 'Desktop', 'Tablet']
regions = ['North', 'South', 'East', 'West']
channels = ['Google Ads', 'Organic', 'Email', 'Social Media']
categories = ['Electronics', 'Fashion', 'Home', 'Beauty', 'Sports']

for i in range(1, 10001):
    user_id = f"USR{i:05d}"
    session_id = f"SES{i:05d}"
    event_time = fake.date_time_between(start_date='-30d', end_date='now')

    for stage in stages:
        if random.random() < probabilities[stage]:

            record = {
                'User_ID': user_id,
                'Session_ID': session_id,
                'Event': stage,
                'Timestamp': event_time.strftime('%Y-%m-%d %H:%M:%S'),
                'Device': random.choice(devices),
                'Region': random.choice(regions),
                'Channel': random.choice(channels),
                'Product_Category': random.choice(categories),
                'Revenue': round(random.uniform(200, 2000), 2) if stage == 'Purchase' else 0,
                'Bounce_Flag': 'Yes'
            }

            data.append(record)

            event_time += timedelta(minutes=random.randint(2, 5))

        else:
            break

# Create dataframe AFTER loop
df = pd.DataFrame(data)



df.loc[df['User_ID'].isin(df[df['Event'] == 'Purchase']['User_ID']), 'Bounce_Flag'] = 'No'

df.to_csv("funnel_analysis_data.csv", index=False)

print('Funnel dataset generated successfully!')